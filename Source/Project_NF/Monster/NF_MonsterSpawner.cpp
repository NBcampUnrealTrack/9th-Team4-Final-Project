#include "Monster/NF_MonsterSpawner.h"

#include "Components/SceneComponent.h"
#include "Monster/NF_Monster.h"
#include "NavigationSystem.h"

ANF_MonsterSpawner::ANF_MonsterSpawner()
{
    bReplicates = true;

    SpawnRootComponent =
        CreateDefaultSubobject<USceneComponent>(
            TEXT("SpawnRootComponent"));

    SetRootComponent(SpawnRootComponent);
}

void ANF_MonsterSpawner::BeginPlay()
{
    Super::BeginPlay();

    if (!HasAuthority()) { return; }

    SpawnInitialMonsters();
}

void ANF_MonsterSpawner::SpawnInitialMonsters()
{
    const int32 SpawnCount = FMath::Min(
        InitialSpawnCount,
        MaxAliveCount);

    for (int32 Index = 0; Index < SpawnCount; ++Index)
    {
        SpawnMonster();
    }
}

bool ANF_MonsterSpawner::SpawnMonster()
{
    if (MonsterClasses.IsEmpty()
        || AliveMonsters.Num() >= MaxAliveCount)
    {
        return false;
    }

    const int32 ClassIndex = FMath::RandRange(
        0,
        MonsterClasses.Num() - 1);

    const TSubclassOf<ANF_Monster> MonsterClass =
        MonsterClasses[ClassIndex];

    if (!MonsterClass)
    {
        return false;
    }

    FVector SpawnLocation = GetActorLocation();

    if (UNavigationSystemV1* NavigationSystem =
    UNavigationSystemV1::GetCurrent(GetWorld()))
    {
        FNavLocation NavigationLocation;

        if (NavigationSystem->GetRandomReachablePointInRadius(
            GetActorLocation(),
            SpawnRadius,
            NavigationLocation))
        {
            SpawnLocation = NavigationLocation.Location;
        }
    }

    const FRotator SpawnRotation(
        0.0f,
        FMath::FRandRange(-180.0f, 180.0f),
        0.0f);

    FActorSpawnParameters SpawnParameters;

    SpawnParameters.Owner = this;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::
        AdjustIfPossibleButAlwaysSpawn;

    ANF_Monster* SpawnedMonster =
        GetWorld()->SpawnActor<ANF_Monster>(
            MonsterClass,
            SpawnLocation,
            SpawnRotation,
            SpawnParameters);

    if (!SpawnedMonster)
    {
        return false;
    }

    SpawnedMonster->OnMonsterDied.AddUniqueDynamic(
        this,
        &ThisClass::HandleMonsterDied);

    AliveMonsters.Add(SpawnedMonster);

    return true;
}

void ANF_MonsterSpawner::HandleMonsterDied(
    ANF_Monster* DeadMonster)
{
    if (!HasAuthority() || !DeadMonster)
    {
        return;
    }

    AliveMonsters.Remove(DeadMonster);

    if (!bRespawnEnabled
        || AliveMonsters.Num() >= MaxAliveCount)
    {
        return;
    }

    ++PendingRespawnCount;

    if (!GetWorldTimerManager().IsTimerActive(
        RespawnTimerHandle))
    {
        GetWorldTimerManager().SetTimer(
            RespawnTimerHandle,
            this,
            &ThisClass::SpawnPendingMonster,
            RespawnDelay,
            false);
    }
}

void ANF_MonsterSpawner::SpawnPendingMonster()
{
    if (PendingRespawnCount <= 0)
    {
        return;
    }

    if (SpawnMonster())
    {
        --PendingRespawnCount;
    }

    if (PendingRespawnCount > 0)
    {
        GetWorldTimerManager().SetTimer(
            RespawnTimerHandle,
            this,
            &ThisClass::SpawnPendingMonster,
            RespawnDelay,
            false);
    }
}