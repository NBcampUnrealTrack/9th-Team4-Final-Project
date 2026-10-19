#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NF_BossRegistrySubsystem.generated.h"

class ANF_BossBase;

DECLARE_MULTICAST_DELEGATE_OneParam(FNF_OnActiveBossChanged, ANF_BossBase*);

/** Tracks boss actors in one world and announces which boss the local UI should observe. */
UCLASS()
class PROJECT_NF_API UNF_BossRegistrySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterBoss(ANF_BossBase* Boss);
	void UnregisterBoss(ANF_BossBase* Boss);

	ANF_BossBase* GetActiveBoss() const { return ActiveBoss.Get(); }

	FNF_OnActiveBossChanged OnActiveBossChanged;

private:
	void SetActiveBoss(ANF_BossBase* Boss);

	TArray<TWeakObjectPtr<ANF_BossBase>> RegisteredBosses;
	TWeakObjectPtr<ANF_BossBase> ActiveBoss;
};
