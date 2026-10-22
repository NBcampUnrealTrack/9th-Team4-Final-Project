#include "GamePlay/Combat/Boss/NF_BossSpawner.h"

#include "Components/SceneComponent.h"
#include "Core/NF_GameState.h"
#include "Engine/World.h"
#include "GamePlay/Combat/Boss/NF_BossBase.h"
#include "GamePlay/Combat/Boss/NF_BossRegistrySubsystem.h"

ANF_BossSpawner::ANF_BossSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("SpawnRoot")));
}

void ANF_BossSpawner::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority() || GetNetMode() == NM_Client) return;

	Registry = GetWorld()->GetSubsystem<UNF_BossRegistrySubsystem>();
	if (!BossClass || !Registry.IsValid() ||
		!Registry->RegisterBossSpawnHandler(FNF_RequestBossSpawn::CreateUObject(this, &ThisClass::SpawnBoss)))
	{
		UE_LOG(LogTemp, Error, TEXT("[BossSpawn] Core registration failed for %s. Check BossClass, GameMode and duplicate spawners."), *GetName());
	}
}

void ANF_BossSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Registry.IsValid())
	{
		Registry->UnregisterBossSpawnHandler(this);
	}
	Registry.Reset();
	Super::EndPlay(EndPlayReason);
}

ANF_BossBase* ANF_BossSpawner::SpawnBoss()
{
	UWorld* World = GetWorld();
	if (!HasAuthority() || GetNetMode() == NM_Client || !World || !World->HasBegunPlay() ||
		IsActorBeingDestroyed() || bSpawnRequested || !BossClass || !Registry.IsValid()) return nullptr;

	// Set before SpawnActor: construction and BeginPlay can invoke external callbacks.
	bSpawnRequested = true;
	if (IsValid(Registry->GetActiveBoss()))
	{
		UE_LOG(LogTemp, Error, TEXT("[BossSpawn] A boss already exists; refusing to create a second encounter boss."));
		return nullptr;
	}

	FActorSpawnParameters Parameters;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
	ANF_BossBase* Boss = World->SpawnActor<ANF_BossBase>(BossClass, GetActorTransform(), Parameters);
	const ANF_GameState* GameState = World->GetGameState<ANF_GameState>();
	// SpawnActor completes construction and BeginPlay synchronously in a running world.
	if (!IsValid(Boss) || IsActorBeingDestroyed() || !Boss->IsReadyForEncounter() ||
		!IsValid(GameState) || GameState->GetMatchPhase() != ENF_MatchPhase::Farming ||
		!Registry.IsValid() || Registry->GetActiveBoss() != Boss)
	{
		if (IsValid(Boss)) Boss->Destroy();
		UE_LOG(LogTemp, Error, TEXT("[BossSpawn] Spawn or GAS/AI initialization failed for %s."), *GetName());
		return nullptr;
	}

	return Boss;
}
