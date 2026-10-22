#pragma once

#include "CoreMinimal.h"
#include "Core/NF_CoreIntegrationTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "NF_BossRegistrySubsystem.generated.h"

class ANF_BossBase;
class ANF_GameMode;

DECLARE_MULTICAST_DELEGATE_OneParam(FNF_OnActiveBossChanged, ANF_BossBase*);

/** Tracks boss actors in one world and announces which boss the local UI should observe. */
UCLASS()
class PROJECT_NF_API UNF_BossRegistrySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	void RegisterBoss(ANF_BossBase* Boss);
	void UnregisterBoss(ANF_BossBase* Boss);

	/** Registers the same UObject callback with Core. Success means both are connected. */
	bool RegisterBossSpawnHandler(FNF_RequestBossSpawn Handler);
	void UnregisterBossSpawnHandler(const UObject* HandlerOwner);

	ANF_BossBase* GetActiveBoss() const { return ActiveBoss.Get(); }

	FNF_OnActiveBossChanged OnActiveBossChanged;

private:
	void SetActiveBoss(ANF_BossBase* Boss);

	TArray<TWeakObjectPtr<ANF_BossBase>> RegisteredBosses;
	TWeakObjectPtr<ANF_BossBase> ActiveBoss;
	FNF_RequestBossSpawn BossSpawnHandler;
	TWeakObjectPtr<ANF_GameMode> SpawnGameMode;
};
