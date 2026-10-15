#include "Core/NF_GameState.h"

#include "Net/UnrealNetwork.h"

ANF_GameState::ANF_GameState()
{
	bReplicates = true;
}

void ANF_GameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANF_GameState, NF_MatchState);
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

void ANF_GameState::OnRep_NFMatchState()
{
	BroadcastMatchStateChanged();
}

void ANF_GameState::BroadcastMatchStateChanged()
{
	OnMatchStateChanged.Broadcast(NF_MatchState);
}