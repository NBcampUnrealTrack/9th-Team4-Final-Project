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
		meta = (ClampMin = "50.0", ToolTip = "Spawner origin around which valid NavMesh points are sampled."))
	float SpawnRadius = 200.0f;

	/** Keeps the initial group from spawning on top of itself. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly,
		Category = "NF|Monster|Spawner",
		meta = (ClampMin = "0.0", ToolTip = "Minimum 2D separation between living monsters from this spawner."))
	float MinimumSpawnSeparation = 60.0f;

	/** Number of NavMesh candidates tried before this spawn request gives up safely. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly,
		Category = "NF|Monster|Spawner",
		meta = (ClampMin = "1", ClampMax = "50"))
	int32 MaxSpawnLocationAttempts = 12;

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
	bool IsSpawnLocationSeparated(const FVector& CandidateLocation) const;
	bool FindSafeSpawnLocation(FVector& OutSpawnLocation) const;
	TSubclassOf<ANF_Monster> GetRandomMonsterClass() const;

	int32 PendingRespawnCount = 0;

	FTimerHandle RespawnTimerHandle;
};
