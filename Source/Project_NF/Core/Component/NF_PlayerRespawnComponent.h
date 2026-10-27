#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/NF_CoreIntegrationTypes.h"
#include "Data/Enum/NF_PlayerPawnState.h"
#include "NF_PlayerRespawnComponent.generated.h"

class AController;
class ANF_GameMode;
class ANF_PlayerState;
class ANF_TeamPlayerStart;
class UNF_PlayerLifeComponent;

enum class ENFPlayerLifeState : uint8;

/**
 * @owner : 조민웅 | 2026-10-27
 * GameMode에 부착하는 서버 전용 컴포넌트.
 * Pawn 생명주기 상태, 사망 구독, 리스폰 예약과 새 Pawn 생성을 담당한다.
 */
UCLASS(ClassGroup = (NF))
class PROJECT_NF_API UNF_PlayerRespawnComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNF_PlayerRespawnComponent();

	void RegisterPlayer(ANF_PlayerState* PlayerState);
	void HandlePlayerRestarted(AController* Controller);
	void HandlePlayerLeaving(ANF_PlayerState* PlayerState);

	void BindAssignedLifeComponents(
		const TArray<UNF_PlayerLifeComponent*>& LifeComponents);

	void NotifyTeamsReady();
	void LockRespawnsAfterBossDefeat();
	void Shutdown();

	ENF_PlayerPawnState GetPlayerPawnState(
		const ANF_PlayerState* PlayerState) const;

	FNF_EvaluateRespawnPermission EvaluateRespawnPermission;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "NF|Respawn", meta = (ClampMin = "0.1"))
	float RespawnDelaySeconds = 30.0f;

	TMap<TWeakObjectPtr<ANF_PlayerState>, ENF_PlayerPawnState> PlayerPawnStates;

	TMap<
		TWeakObjectPtr<ANF_PlayerState>,
		TWeakObjectPtr<UNF_PlayerLifeComponent>> BoundLifeComponents;

	TMap<TWeakObjectPtr<ANF_PlayerState>, FTimerHandle> RespawnTimerHandles;

	bool bTeamsReady = false;
	bool bRespawnLockedByBossDefeat = false;
	bool bStopped = false;

	bool HasServerAuthority() const;
	ANF_GameMode* GetNFGameMode() const;

	bool SetPlayerPawnState(
		ANF_PlayerState* PlayerState,
		ENF_PlayerPawnState NewState);

	void RemovePlayerPawnState(const ANF_PlayerState* PlayerState);

	void BindPlayerLifeComponent(UNF_PlayerLifeComponent* LifeComponent);
	void UnbindPlayerLifeComponent(ANF_PlayerState* PlayerState);

	UFUNCTION()
	void HandlePlayerLifeStateChanged(
		UNF_PlayerLifeComponent* SourceLifeComponent,
		ENFPlayerLifeState NewState);

	bool CanRespawnPlayer(const ANF_PlayerState* PlayerState) const;
	void SchedulePlayerRespawn(ANF_PlayerState* PlayerState);
	void ExecutePlayerRespawn(TWeakObjectPtr<ANF_PlayerState> PlayerStateKey);

	void CancelPlayerRespawn(
		ANF_PlayerState* PlayerState,
		bool bMarkEliminated);

	void CancelAllPendingRespawns(bool bMarkEliminated);

	ANF_TeamPlayerStart* FindTeamPlayerStart(int32 TeamId) const;
	void RetryTeamPendingHealthZeroChecks(int32 TeamId);
};
