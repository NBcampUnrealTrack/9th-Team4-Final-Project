#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NF_BossSpawner.generated.h"

class ANF_BossBase;
class UNF_BossRegistrySubsystem;

/** Place one in the persistent battle level and assign a configured boss Blueprint. */
UCLASS(Blueprintable)
class PROJECT_NF_API ANF_BossSpawner : public AActor
{
	GENERATED_BODY()

public:
	ANF_BossSpawner();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, Category = "NF|Boss|Spawn")
	TSubclassOf<ANF_BossBase> BossClass;

private:
	ANF_BossBase* SpawnBoss();
	TWeakObjectPtr<UNF_BossRegistrySubsystem> Registry;
	bool bSpawnRequested = false;
};
