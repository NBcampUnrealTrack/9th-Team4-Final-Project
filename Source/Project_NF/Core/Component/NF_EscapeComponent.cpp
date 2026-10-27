#include "Core/Component/NF_EscapeComponent.h"

#include "AbilitySystemComponent.h"
#include "Core/NF_GameState.h"
#include "Engine/World.h"
#include "GAS/Tags/NFTag_LifeState.h"
#include "GamePlay/EscapeZone/NF_EscapeZone.h"
#include "Player/State/NF_PlayerState.h"
#include "TimerManager.h"

UNF_EscapeComponent::UNF_EscapeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

bool UNF_EscapeComponent::HasServerAuthority() const
{
	return IsValid(GetOwner()) && GetOwner()->HasAuthority();
}

double UNF_EscapeComponent::GetServerTime() const
{
	const UWorld* World = GetWorld();
	if (!IsValid(World)) return 0.0;

	const ANF_GameState* GameState = World->GetGameState<ANF_GameState>();
	if (!IsValid(GameState)) return 0.0;

	return GameState->GetServerWorldTimeSeconds();
}

bool UNF_EscapeComponent::TryStartEscape(ANF_EscapeZone* Zone, ANF_PlayerState* Requester)
{
	if (!HasServerAuthority() || bEscaping || !IsValid(Zone) || !IsValid(Requester)) return false;
	if (!EvaluateEscapeRequest.IsBound()) return false;

	const FNF_EscapeValidationResult Validation = EvaluateEscapeRequest.Execute(Zone, Requester);
	if (!IsValid(Validation.Holder) || !IsValid(Validation.Teammate)) return false;

	// 최초 상호작용은 팀원이 범위 안에 있거나 이미 사망한 경우에만 허용한다.
	if (Validation.State != ENF_EscapeValidationState::TeamReady &&
		Validation.State != ENF_EscapeValidationState::SoloReady)
	{
		return false;
	}

	ActiveZone = Zone;
	ActiveRequester = Requester;
	StartingHolder = Validation.Holder;
	ActiveTeammate = Validation.Teammate;
	EscapingTeamId = Validation.Holder->NF_TeamID;
	bEscaping = true;

	BindTeammateDeathEvent();

	GetWorld()->GetTimerManager().SetTimer(
		MonitorTimerHandle,
		this,
		&ThisClass::MonitorEscape,
		0.1f,
		true);

	if (Validation.State == ENF_EscapeValidationState::SoloReady)
	{
		StartCountdown(ENF_EscapeState::CountingSolo, Zone->GetSoloEscapeDuration());
	}
	else
	{
		StartCountdown(ENF_EscapeState::CountingWithTeammate, Zone->GetTeamEscapeDuration());
	}

	return true;
}

void UNF_EscapeComponent::StartCountdown(const ENF_EscapeState NewState, float Duration)
{
	if (!bEscaping || !ActiveZone.IsValid()) return;

	Duration = FMath::Max(0.1f, Duration);
	EscapeState = NewState;
	TotalDuration = Duration;
	RemainingTime = Duration;
	EndServerTime = GetServerTime() + Duration;

	GetWorld()->GetTimerManager().ClearTimer(CompletionTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(
		CompletionTimerHandle,
		this,
		&ThisClass::CompleteEscape,
		Duration,
		false);

	PublishProgress();
}

void UNF_EscapeComponent::PauseForTeammate()
{
	if (!bEscaping || EscapeState != ENF_EscapeState::CountingWithTeammate) return;

	RemainingTime = static_cast<float>(EndServerTime - GetServerTime());
	RemainingTime = FMath::Max(0.0f, RemainingTime);

	GetWorld()->GetTimerManager().ClearTimer(CompletionTimerHandle);
	EscapeState = ENF_EscapeState::PausedForTeammate;
	EndServerTime = 0.0;

	PublishProgress();
}

void UNF_EscapeComponent::ResumeWithTeammate()
{
	if (!bEscaping || EscapeState != ENF_EscapeState::PausedForTeammate) return;

	EscapeState = ENF_EscapeState::CountingWithTeammate;
	EndServerTime = GetServerTime() + RemainingTime;

	GetWorld()->GetTimerManager().SetTimer(
		CompletionTimerHandle,
		this,
		&ThisClass::CompleteEscape,
		FMath::Max(0.001f, RemainingTime),
		false);

	PublishProgress();
}

void UNF_EscapeComponent::ResetToTeamWaiting()
{
	if (!bEscaping || !ActiveZone.IsValid()) return;

	GetWorld()->GetTimerManager().ClearTimer(CompletionTimerHandle);
	EscapeState = ENF_EscapeState::PausedForTeammate;
	TotalDuration = ActiveZone->GetTeamEscapeDuration();
	RemainingTime = TotalDuration;
	EndServerTime = 0.0;

	PublishProgress();
}

void UNF_EscapeComponent::SwitchToSoloCountdown()
{
	if (!bEscaping || !ActiveZone.IsValid()) return;

	// 팀 탈출 진행도는 폐기하고 단독 탈출 시간을 처음부터 적용한다.
	StartCountdown(ENF_EscapeState::CountingSolo, ActiveZone->GetSoloEscapeDuration());
}

void UNF_EscapeComponent::EvaluateActiveEscape()
{
	if (!HasServerAuthority() ||
		!bEscaping ||
		!EvaluateEscapeRequest.IsBound() ||
		!ActiveZone.IsValid() ||
		!ActiveRequester.IsValid() ||
		!StartingHolder.IsValid())
	{
		CancelEscape();
		return;
	}

	const FNF_EscapeValidationResult Validation = EvaluateEscapeRequest.Execute(
		ActiveZone.Get(), ActiveRequester.Get());

	if (!IsValid(Validation.Holder) ||
		Validation.Holder != StartingHolder.Get() ||
		Validation.Holder->NF_TeamID != EscapingTeamId)
	{
		CancelEscape();
		return;
	}

	switch (Validation.State)
	{
	case ENF_EscapeValidationState::TeamReady:
		if (EscapeState == ENF_EscapeState::PausedForTeammate)
		{
			ResumeWithTeammate();
		}
		else if (EscapeState == ENF_EscapeState::CountingSolo)
		{
			StartCountdown(ENF_EscapeState::CountingWithTeammate, ActiveZone->GetTeamEscapeDuration());
		}
		break;

	case ENF_EscapeValidationState::WaitingForTeammate:
		if (EscapeState == ENF_EscapeState::CountingWithTeammate)
		{
			PauseForTeammate();
		}
		else if (EscapeState == ENF_EscapeState::CountingSolo)
		{
			ResetToTeamWaiting();
		}
		break;

	case ENF_EscapeValidationState::SoloReady:
		if (EscapeState != ENF_EscapeState::CountingSolo)
		{
			SwitchToSoloCountdown();
		}
		break;

	case ENF_EscapeValidationState::Invalid:
	default:
		CancelEscape();
		break;
	}
}

void UNF_EscapeComponent::MonitorEscape()
{
	EvaluateActiveEscape();
}

void UNF_EscapeComponent::CompleteEscape()
{
	EvaluateActiveEscape();
	if (!bEscaping) return;
	if (EscapeState == ENF_EscapeState::PausedForTeammate) return;

	if (EscapeState != ENF_EscapeState::CountingWithTeammate &&
		EscapeState != ENF_EscapeState::CountingSolo)
	{
		CancelEscape();
		return;
	}

	const double Remaining = EndServerTime - GetServerTime();
	if (Remaining > 0.0)
	{
		GetWorld()->GetTimerManager().SetTimer(
			CompletionTimerHandle,
			this,
			&ThisClass::CompleteEscape,
			FMath::Max(0.001f, static_cast<float>(Remaining)),
			false);
		return;
	}

	const int32 WinningTeamId = EscapingTeamId;

	// 성공 콜백에서 매치 정리가 재진입해도 진행 중 상태가 남지 않도록 먼저 해제한다.
	CancelEscape();
	OnEscapeSucceeded.Broadcast(WinningTeamId);
}

void UNF_EscapeComponent::PublishProgress()
{
	ANF_EscapeZone* Zone = ActiveZone.Get();
	if (!IsValid(Zone)) return;

	FNF_EscapeProgress Progress;
	Progress.bInProgress = bEscaping;
	Progress.State = EscapeState;
	Progress.TeamId = EscapingTeamId;
	Progress.TotalDuration = TotalDuration;
	Progress.EndServerTime = EndServerTime;

	if (EscapeState == ENF_EscapeState::PausedForTeammate)
	{
		Progress.bPaused = true;
		Progress.RemainingTime = RemainingTime;
		Progress.EndServerTime = 0.0;
	}
	else
	{
		Progress.bPaused = false;
		const double Remaining = EndServerTime - GetServerTime();
		Progress.RemainingTime = FMath::Max(0.0f, static_cast<float>(Remaining));
	}

	Zone->AuthoritySetProgress(Progress);

	if (ANF_GameState* GameState = GetWorld()->GetGameState<ANF_GameState>())
	{
		GameState->AuthoritySetEscapeProgress(Progress);
	}
}

void UNF_EscapeComponent::BindTeammateDeathEvent()
{
	UnbindTeammateDeathEvent();

	ANF_PlayerState* Teammate = ActiveTeammate.Get();
	if (!IsValid(Teammate)) return;

	UAbilitySystemComponent* ASC = Teammate->GetAbilitySystemComponent();
	if (!IsValid(ASC)) return;

	TeammateASC = ASC;
	TeammateDeathTagHandle = ASC->RegisterGameplayTagEvent(
		NFTag_LifeState::State_Life_Dead,
		EGameplayTagEventType::NewOrRemoved).AddUObject(
		this,
		&ThisClass::HandleTeammateDeathTagChanged);
}

void UNF_EscapeComponent::UnbindTeammateDeathEvent()
{
	UAbilitySystemComponent* ASC = TeammateASC.Get();
	if (IsValid(ASC) && TeammateDeathTagHandle.IsValid())
	{
		ASC->RegisterGameplayTagEvent(
			NFTag_LifeState::State_Life_Dead,
			EGameplayTagEventType::NewOrRemoved).Remove(TeammateDeathTagHandle);
	}

	TeammateDeathTagHandle.Reset();
	TeammateASC.Reset();
}

void UNF_EscapeComponent::HandleTeammateDeathTagChanged(const FGameplayTag Tag, const int32)
{
	if (!HasServerAuthority() || !bEscaping) return;
	if (Tag != NFTag_LifeState::State_Life_Dead) return;

	// 사망 태그의 추가와 제거를 모두 현재 규칙으로 다시 판정한다.
	EvaluateActiveEscape();
}

void UNF_EscapeComponent::CancelEscape()
{
	if (!HasServerAuthority()) return;

	UWorld* World = GetWorld();
	if (IsValid(World))
	{
		World->GetTimerManager().ClearTimer(MonitorTimerHandle);
		World->GetTimerManager().ClearTimer(CompletionTimerHandle);
	}

	UnbindTeammateDeathEvent();

	ANF_EscapeZone* PreviousZone = ActiveZone.Get();
	bEscaping = false;
	EscapeState = ENF_EscapeState::Idle;
	ActiveZone.Reset();
	ActiveRequester.Reset();
	StartingHolder.Reset();
	ActiveTeammate.Reset();
	EscapingTeamId = INDEX_NONE;
	EndServerTime = 0.0;
	TotalDuration = 0.0f;
	RemainingTime = 0.0f;

	const FNF_EscapeProgress EmptyProgress;

	if (IsValid(PreviousZone))
	{
		PreviousZone->AuthoritySetProgress(EmptyProgress);
	}

	if (UWorld* World = GetWorld())
	{
		if (ANF_GameState* GameState = World->GetGameState<ANF_GameState>())
		{
			GameState->AuthoritySetEscapeProgress(EmptyProgress);
		}
	}
}

void UNF_EscapeComponent::HandlePlayerLeaving(ANF_PlayerState* PlayerState, APawn*)
{
	if (!bEscaping || !IsValid(PlayerState)) return;

	// 접속 종료는 사망으로 간주하지 않고 해당 팀의 진행을 취소한다.
	if (PlayerState->NF_TeamID == EscapingTeamId)
	{
		CancelEscape();
	}
}

void UNF_EscapeComponent::HandleRangeExited(ANF_EscapeZone* Zone, ANF_PlayerState* PlayerState)
{
	if (!bEscaping ||
		ActiveZone.Get() != Zone ||
		!IsValid(PlayerState) ||
		PlayerState->NF_TeamID != EscapingTeamId)
	{
		return;
	}

	// 보유자 이탈은 취소, 생존 팀원 이탈은 일시정지로 판정된다.
	EvaluateActiveEscape();
}

void UNF_EscapeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelEscape();
	EvaluateEscapeRequest.Unbind();
	OnEscapeSucceeded.Clear();
	Super::EndPlay(EndPlayReason);
}
