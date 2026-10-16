#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NF_MatchDepartureComponent.generated.h"

class ANF_PlayerState;

DECLARE_MULTICAST_DELEGATE(FNF_OnDepartureLimitReached);

/**
 * @owner : 민웅 | 2026-10-16
 * 매치 참가자의 누적 이탈을 집계. 이탈 한도에 도달하면 GameMode에 알림.
 */
UCLASS()
class PROJECT_NF_API UNF_MatchDepartureComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNF_MatchDepartureComponent();

	// Build 시작 시 플레이어 대상 추적
	bool StartTracking(const TArray<ANF_PlayerState*>& Participants);
	
	void NotifyPlayerLeft(TWeakObjectPtr<ANF_PlayerState> PlayerState);

	// 정상 종료, 매치 중단 시 추가 집계를 중지
	void StopTracking();

	int32 GetRemainingPlayerCount() const;

	FNF_OnDepartureLimitReached OnDepartureLimitReached;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(EditDefaultsOnly,Category = "NF|Match|Departure",meta = (ClampMin = "1"))
	int32 DepartureLimit = 2;

	TSet<TWeakObjectPtr<ANF_PlayerState>> RemainingPlayers;

	int32 DepartedPlayerCount = 0;

	bool bTracking = false;

	bool HasServerAuthority() const;
};