#include "Core/Component/NF_EscapeComponent.h"

#include "Core/NF_GameState.h"
#include "Engine/World.h"
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

bool UNF_EscapeComponent::TryStartEscape(ANF_EscapeZone* Zone, ANF_PlayerState* Requester)
{
	if (!HasServerAuthority() || bEscaping || !IsValid(Zone) || !IsValid(Requester)) return false;
	if (!ValidateEscapeRequest.IsBound()) return false;

	ANF_PlayerState* Holder = nullptr;
	if (!ValidateEscapeRequest.Execute(Zone, Requester, Holder) || !IsValid(Holder)) return false;

	const ANF_GameState* GameState = GetWorld()->GetGameState<ANF_GameState>();
	if (!IsValid(GameState)) return false;

	const float Duration = Zone->GetEscapeDuration();
	ActiveZone = Zone;
	ActiveRequester = Requester;
	StartingHolder = Holder;
	EscapingTeamId = Holder->NF_TeamID;
	EndServerTime = GameState->GetServerWorldTimeSeconds() + Duration;
	bEscaping = true;

	GetWorld()->GetTimerManager().SetTimer(MonitorTimerHandle, this, &ThisClass::MonitorEscape, 0.1f, true);
	GetWorld()->GetTimerManager().SetTimer(CompletionTimerHandle, this, &ThisClass::CompleteEscape, Duration, false);

	FNF_EscapeProgress Progress;
	Progress.bInProgress = true;
	Progress.TeamId = EscapingTeamId;
	Progress.EndServerTime = EndServerTime;
	Zone->AuthoritySetProgress(Progress);

	return true;
}

bool UNF_EscapeComponent::ValidateActiveEscape() const
{
	if (!HasServerAuthority() || !bEscaping || !ValidateEscapeRequest.IsBound()) return false;
	if (!ActiveZone.IsValid() || !ActiveRequester.IsValid() || !StartingHolder.IsValid()) return false;

	ANF_PlayerState* CurrentHolder = nullptr;
	if (!ValidateEscapeRequest.Execute(ActiveZone.Get(), ActiveRequester.Get(), CurrentHolder)) return false;

	return IsValid(CurrentHolder) && CurrentHolder == StartingHolder.Get() && CurrentHolder->NF_TeamID == EscapingTeamId;
}

void UNF_EscapeComponent::MonitorEscape()
{
	if (!ValidateActiveEscape())
	{
		CancelEscape();
	}
}

void UNF_EscapeComponent::CompleteEscape()
{
	if (!ValidateActiveEscape())
	{
		CancelEscape();
		return;
	}

	const ANF_GameState* GameState = GetWorld()->GetGameState<ANF_GameState>();
	if (!IsValid(GameState))
	{
		CancelEscape();
		return;
	}

	// 타이머와 서버 시각의 미세한 오차로 일찍 호출되면 남은 시간만 다시 기다린다.
	const double RemainingTime = EndServerTime - GameState->GetServerWorldTimeSeconds();
	if (RemainingTime > 0.0)
	{
		GetWorld()->GetTimerManager().SetTimer(CompletionTimerHandle, this, &ThisClass::CompleteEscape, FMath::Max(0.001f, static_cast<float>(RemainingTime)), false);
		return;
	}

	const int32 WinningTeamId = EscapingTeamId;

	// 성공 콜백에서 매치 정리가 재진입해도 진행 중 상태가 남지 않도록 먼저 해제.
	CancelEscape();
	OnEscapeSucceeded.Broadcast(WinningTeamId);
}

void UNF_EscapeComponent::CancelEscape()
{
	if (!HasServerAuthority()) return;

	GetWorld()->GetTimerManager().ClearTimer(MonitorTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(CompletionTimerHandle);

	ANF_EscapeZone* PreviousZone = ActiveZone.Get();
	bEscaping = false;
	ActiveZone.Reset();
	ActiveRequester.Reset();
	StartingHolder.Reset();
	EscapingTeamId = INDEX_NONE;
	EndServerTime = 0.0;

	if (IsValid(PreviousZone))
	{
		PreviousZone->AuthoritySetProgress(FNF_EscapeProgress());
	}
}

void UNF_EscapeComponent::HandlePlayerLeaving(ANF_PlayerState* PlayerState, APawn* LastPawn)
{
	if (bEscaping && IsValid(PlayerState) && PlayerState->NF_TeamID == EscapingTeamId)
	{
		CancelEscape();
	}
}

void UNF_EscapeComponent::HandleRangeExited(ANF_EscapeZone* Zone, ANF_PlayerState* PlayerState)
{
	if (bEscaping && ActiveZone.Get() == Zone && IsValid(PlayerState) && PlayerState->NF_TeamID == EscapingTeamId)
	{
		CancelEscape();
	}
}

void UNF_EscapeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelEscape();
	ValidateEscapeRequest.Unbind();
	OnEscapeSucceeded.Clear();
	Super::EndPlay(EndPlayReason);
}
