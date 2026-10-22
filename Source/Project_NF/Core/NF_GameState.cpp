#include "Core/NF_GameState.h"

#include "NF_BuildShopAccessPolicy.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Net/UnrealNetwork.h"
#include "Player/State/NF_PlayerState.h"
#include "UI/SubSystem/NF_UIManagerSubsystem.h"

ANF_GameState::ANF_GameState()
{
	bReplicates = true;
}

void ANF_GameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANF_GameState, NF_MatchState);
	DOREPLIFETIME(ANF_GameState, KeyPublicState);
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

bool ANF_GameState::AuthoritySetTimedMatchPhase(ENF_MatchPhase NewPhase,float Duration)
{
	if (!HasAuthority()) { return false; }

	//제한시간이 없는 페이즈는 AuthoritySetUntimedMatchPhase() 사용
	if (Duration <= 0.0f) { return false; }

	NF_MatchState.Phase = NewPhase;
	NF_MatchState.bHasTimeLimit = true;
	NF_MatchState.PhaseEndServerTime = GetServerWorldTimeSeconds() + Duration;

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

	BroadcastMatchStateChanged();
	ForceNetUpdate();

	return true;
}

bool ANF_GameState::AuthorityFinishMatch(const FNF_MatchResult& Result)
{
	if (!HasAuthority()){return false;}

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

	KeyPublicState = NewState;
	const FNF_KeyPublicState PublishedState = KeyPublicState;
	OnKeyPublicStateChanged.Broadcast(PublishedState);
	ForceNetUpdate();
	return true;
}

void ANF_GameState::OnRep_KeyPublicState()
{
	const FNF_KeyPublicState PublishedState = KeyPublicState;
	OnKeyPublicStateChanged.Broadcast(PublishedState);
}

void ANF_GameState::MulticastShowAnnouncement_Implementation(const FText& Message, const float Duration)
{
	UGameInstance* GameInstance = GetGameInstance();
	if (!IsValid(GameInstance))
	{
		return;
	}

	for (ULocalPlayer* LocalPlayer : GameInstance->GetLocalPlayers())
	{
		if (!IsValid(LocalPlayer))
		{
			continue;
		}

		if (UNF_UIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UNF_UIManagerSubsystem>())
		{
			UIManager->ShowAnnouncement(Message, Duration);
		}
	}
}
