#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NF_MonsterSpawner.generated.h"

class ANF_Monster;
class USceneComponent;

UCLASS()
class PROJECT_NF_API ANF_MonsterSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	ANF_MonsterSpawner();
	
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleMonsterDied(ANF_Monster* DeadMonster);

	void SpawnInitialMonsters();
	bool SpawnMonster();
	void SpawnPendingMonster();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		Category = "NF|Monster|Spawner")
	TObjectPtr<USceneComponent> SpawnRootComponent;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly,
		Category = "NF|Monster|Spawner")
	TArray<TSubclassOf<ANF_Monster>> MonsterClasses;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly,
		Category = "NF|Monster|Spawner",
		meta = (ClampMin = "0"))
	int32 InitialSpawnCount = 3;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly,
		Category = "NF|Monster|Spawner",
		meta = (ClampMin = "1"))
	int32 MaxAliveCount = 3;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly,
		Category = "NF|Monster|Spawner",
		meta = (ClampMin = "0.0"))
	float SpawnRadius = 300.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly,
		Category = "NF|Monster|Spawner")
	bool bRespawnEnabled = true;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly,
		Category = "NF|Monster|Spawner",
		meta = (ClampMin = "0.0"))
	float RespawnDelay = 6.0f;

	UPROPERTY(Transient)
	TSet<TObjectPtr<ANF_Monster>> AliveMonsters;

private:
	int32 PendingRespawnCount = 0;

	FTimerHandle RespawnTimerHandle;
};
