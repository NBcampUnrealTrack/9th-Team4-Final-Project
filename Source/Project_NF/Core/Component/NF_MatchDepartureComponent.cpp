#include "Core/Component/NF_MatchDepartureComponent.h"

#include "GameFramework/Actor.h"
#include "Player/State/NF_PlayerState.h"

UNF_MatchDepartureComponent::UNF_MatchDepartureComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UNF_MatchDepartureComponent::HasServerAuthority() const
{
	const AActor* Owner = GetOwner();

	return IsValid(Owner) && Owner->HasAuthority();
}

bool UNF_MatchDepartureComponent::StartTracking(const TArray<ANF_PlayerState*>& Participants)
{
	if (!HasServerAuthority()) { return false; }
	if (bTracking || Participants.IsEmpty()) { return false; }
	if (DepartureLimit <= 0) { return false; }

	TSet<TWeakObjectPtr<ANF_PlayerState>> NewParticipants;

	for (ANF_PlayerState* PlayerState : Participants)
	{
		if (!IsValid(PlayerState)) { return false; }
		if (PlayerState->GetWorld() != GetWorld()) { return false; }

		const TWeakObjectPtr<ANF_PlayerState> Key(PlayerState);

		
		if (NewParticipants.Contains(Key)) {return false;}

		NewParticipants.Add(Key);
	}

	RemainingPlayers = MoveTemp(NewParticipants);
	DepartedPlayerCount = 0;
	bTracking = true;

	return true;
}

void UNF_MatchDepartureComponent::NotifyPlayerLeft(const TWeakObjectPtr<ANF_PlayerState> PlayerState)
{
	if (!HasServerAuthority() || !bTracking) return;

	// 이미 집계한 퇴장은 무시
	if (RemainingPlayers.Remove(PlayerState) == 0) return;

	++DepartedPlayerCount;

	UE_LOG(LogTemp,Warning,TEXT("매치 참가자 이탈: 누적 %d명, 남은 참가자 %d명"),DepartedPlayerCount,RemainingPlayers.Num());

	if (DepartedPlayerCount >= DepartureLimit)
	{
		//기준 퇴장 인원 달성 시 추가 추적 중지
		bTracking = false;

		OnDepartureLimitReached.Broadcast();
	}
}

void UNF_MatchDepartureComponent::StopTracking()
{
	if (!HasServerAuthority()) return;

	bTracking = false;
}

int32 UNF_MatchDepartureComponent::GetRemainingPlayerCount() const
{
	return RemainingPlayers.Num();
}

void UNF_MatchDepartureComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bTracking = false;

	RemainingPlayers.Empty();
	OnDepartureLimitReached.Clear();

	Super::EndPlay(EndPlayReason);
}
