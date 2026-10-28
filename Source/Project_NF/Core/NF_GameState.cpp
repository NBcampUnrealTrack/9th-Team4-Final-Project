#include "Core/NF_GameState.h"

#include "NF_BuildShopAccessPolicy.h"
#include "Net/UnrealNetwork.h"
#include "Player/State/NF_PlayerState.h"

ANF_GameState::ANF_GameState()
{
	bReplicates = true;
}

void ANF_GameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANF_GameState, NF_MatchState);
	DOREPLIFETIME(ANF_GameState, KeyPublicState);
	DOREPLIFETIME(ANF_GameState, EscapeProgress);
}

void ANF_GameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);

	ANF_PlayerState* NFPlayerState = Cast<ANF_PlayerState>(PlayerState);
	if (IsValid(NFPlayerState) && PlayerArray.Contains(PlayerState))
	{
		// PlayerState 담당이 제공하는 팀 변경 알림. 중복 구독을 방지한다.
		NFPlayerState->OnTeamIDChanged.RemoveAll(this);
		NFPlayerState->OnTeamIDChanged.AddUObject(this, &ThisClass::HandlePlayerTeamChanged);
	}

	OnTeamRosterChanged.Broadcast();
}

void ANF_GameState::RemovePlayerState(APlayerState* PlayerState)
{
	ANF_PlayerState* NFPlayerState = Cast<ANF_PlayerState>(PlayerState);
	if (IsValid(NFPlayerState))
	{
		NFPlayerState->OnTeamIDChanged.RemoveAll(this);
	}

	Super::RemovePlayerState(PlayerState);
	OnTeamRosterChanged.Broadcast();
}

void ANF_GameState::HandlePlayerTeamChanged()
{
	OnTeamRosterChanged.Broadcast();
}

void ANF_GameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (const TObjectPtr<APlayerState>& PlayerState : PlayerArray)
	{
		ANF_PlayerState* NFPlayerState = Cast<ANF_PlayerState>(PlayerState.Get());
		if (IsValid(NFPlayerState))
		{
			NFPlayerState->OnTeamIDChanged.RemoveAll(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

FNF_MatchState ANF_GameState::GetMatchState() const
{
	return NF_MatchState;
}

ENF_MatchPhase ANF_GameState::GetMatchPhase() const
{
	return NF_MatchState.Phase;
}

double ANF_GameState::GetRemainingPhaseTime() const
{
	if (NF_MatchState.bHasTimeLimit == false) { return 0.0; }

	return FMath::Max(0.0, NF_MatchState.PhaseEndServerTime - GetServerWorldTimeSeconds());
}

double ANF_GameState::GetRemainingConclusionTime() const
{
	if (NF_MatchState.Phase != ENF_MatchPhase::Finished || NF_MatchState.ConclusionEndServerTime <= 0.0)
	{
		return 0.0;
	}

	return FMath::Max(0.0, NF_MatchState.ConclusionEndServerTime - GetServerWorldTimeSeconds());
}

bool ANF_GameState::AuthoritySetTimedMatchPhase(ENF_MatchPhase NewPhase,float Duration)
{
	if (!HasAuthority()) { return false; }

	//제한시간이 없는 페이즈는 AuthoritySetUntimedMatchPhase() 사용
	if (Duration <= 0.0f) { return false; }

	NF_MatchState.Phase = NewPhase;
	NF_MatchState.bHasTimeLimit = true;
	NF_MatchState.PhaseEndServerTime = GetServerWorldTimeSeconds() + Duration;
	NF_MatchState.ConclusionEndServerTime = 0.0;

	BroadcastMatchStateChanged();
	ForceNetUpdate();

	return true;
}

bool ANF_GameState::AuthoritySetUntimedMatchPhase(ENF_MatchPhase NewPhase)
{
	if (!HasAuthority()) { return false; }

	NF_MatchState.Phase = NewPhase;
	NF_MatchState.bHasTimeLimit = false;
	NF_MatchState.PhaseEndServerTime = 0.0;
	NF_MatchState.ConclusionEndServerTime = 0.0;

	BroadcastMatchStateChanged();
	ForceNetUpdate();

	return true;
}

bool ANF_GameState::AuthorityFinishMatch(const FNF_MatchResult& Result, const float ConclusionDuration)
{
	if (!HasAuthority()){return false;}
	if (!FMath::IsFinite(ConclusionDuration) || ConclusionDuration <= 0.0f){return false;}

	if (NF_MatchState.Result.Outcome != ENF_MatchOutcome::None){return false;}

	if (Result.Outcome == ENF_MatchOutcome::None ||Result.EndReason == ENF_MatchEndReason::None){return false;}

	if (Result.Outcome == ENF_MatchOutcome::Victory)
	{
		if (Result.WinningTeamId <= 0 ||Result.EndReason != ENF_MatchEndReason::EscapeSucceeded){return false;}
	}
	else if (Result.Outcome == ENF_MatchOutcome::Aborted)
	{
		if (Result.WinningTeamId != INDEX_NONE ||Result.EndReason == ENF_MatchEndReason::EscapeSucceeded){return false;}
	}

	NF_MatchState.Result = Result;
	NF_MatchState.Phase = ENF_MatchPhase::Finished;
	NF_MatchState.bHasTimeLimit = false;
	NF_MatchState.PhaseEndServerTime = 0.0;
	NF_MatchState.ConclusionEndServerTime = GetServerWorldTimeSeconds() + ConclusionDuration;

	BroadcastMatchStateChanged();
	ForceNetUpdate();

	return true;
}

bool ANF_GameState::IsBuildShopAllowed(ENF_BuildShopType ShopType) const
{
	return FNF_BuildShopAccessPolicy::IsShopAllowed(NF_MatchState.Phase,ShopType);
}

void ANF_GameState::OnRep_NFMatchState()
{
	BroadcastMatchStateChanged();
}

void ANF_GameState::BroadcastMatchStateChanged()
{
	OnMatchStateChanged.Broadcast(NF_MatchState);
}

FNF_KeyPublicState ANF_GameState::GetKeyPublicState() const
{
	return KeyPublicState;
}

bool ANF_GameState::AuthoritySetKeyPublicState(const FNF_KeyPublicState& NewState)
{
	if (!HasAuthority()) { return false; }

	switch (NewState.State)
	{
	case ENF_KeyState::Held:
		if (!IsValid(NewState.Holder) || NewState.Holder->GetWorld() != GetWorld()) { return false; }
		break;
	case ENF_KeyState::Unassigned:
	case ENF_KeyState::Dropped:
	case ENF_KeyState::Removed:
		if (NewState.Holder != nullptr) { return false; }
		break;
	default:
		return false;
	}

	if (KeyPublicState.State == NewState.State && KeyPublicState.Holder == NewState.Holder) { return true; }

	// Delegate 호출 중 상태 변경이 재진입해도 문구를 유지하도록 미리 확정한다.
	FText AnnouncementMessage;
	if (GetMatchPhase() == ENF_MatchPhase::Escape)
	{
		switch (NewState.State)
		{
		case ENF_KeyState::Held:
		{
			const FString HolderName = NewState.Holder->GetPlayerName();
			if (HolderName.IsEmpty())
			{
				AnnouncementMessage = NSLOCTEXT(
					"NFGameFlow", "KeyAcquiredUnknown", "누군가 탈출 열쇠를 획득했습니다.");
			}
			else
			{
				AnnouncementMessage = FText::Format(
					NSLOCTEXT("NFGameFlow", "KeyAcquired", "{0}님이 탈출 열쇠를 획득했습니다."),
					FText::FromString(HolderName));
			}
			break;
		}
		case ENF_KeyState::Dropped:
			AnnouncementMessage = NSLOCTEXT(
				"NFGameFlow", "KeyDropped", "탈출 열쇠가 바닥에 떨어졌습니다.");
			break;
		case ENF_KeyState::Unassigned:
		case ENF_KeyState::Removed:
		default:
			break;
		}
	}

	KeyPublicState = NewState;
	const FNF_KeyPublicState PublishedState = KeyPublicState;

	// 아래 Delegate에서 재진입하기 전에 확정된 변경 알림을 발행한다.
	if (!AnnouncementMessage.IsEmpty())
	{
		MulticastShowAnnouncement(AnnouncementMessage, 3.0f);
	}

	OnKeyPublicStateChanged.Broadcast(PublishedState);
	ForceNetUpdate();
	return true;
}

void ANF_GameState::OnRep_KeyPublicState()
{
	const FNF_KeyPublicState PublishedState = KeyPublicState;
	OnKeyPublicStateChanged.Broadcast(PublishedState);
}

FNF_EscapeProgress ANF_GameState::GetEscapeProgress() const
{
	return EscapeProgress;
}

float ANF_GameState::GetRemainingEscapeTime() const
{
	if (!EscapeProgress.bInProgress)
	{
		return 0.0f;
	}

	if (EscapeProgress.bPaused)
	{
		return FMath::Max(0.0f, EscapeProgress.RemainingTime);
	}

	const double RemainingTime = EscapeProgress.EndServerTime - GetServerWorldTimeSeconds();
	return FMath::Max(0.0f, static_cast<float>(RemainingTime));
}

bool ANF_GameState::AuthoritySetEscapeProgress(const FNF_EscapeProgress& NewProgress)
{
	if (!HasAuthority())
	{
		return false;
	}

	EscapeProgress = NewProgress;
	OnEscapeProgressChanged.Broadcast(EscapeProgress);
	ForceNetUpdate();
	return true;
}

void ANF_GameState::OnRep_EscapeProgress()
{
	OnEscapeProgressChanged.Broadcast(EscapeProgress);
}

void ANF_GameState::MulticastShowAnnouncement_Implementation(const FText& Message, const float Duration)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (Message.IsEmpty())
	{
		return;
	}

	OnAnnouncementReceived.Broadcast(Message, FMath::Max(0.1f, Duration));
}
