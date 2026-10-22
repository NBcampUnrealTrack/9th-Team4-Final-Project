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

    const TSubclassOf<ANF_Monster> MonsterClass = GetRandomMonsterClass();

    if (!MonsterClass)
    {
        return false;
    }

    FActorSpawnParameters SpawnParameters;

    SpawnParameters.Owner = this;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::
        AdjustIfPossibleButDontSpawnIfColliding;

    for (int32 AttemptIndex = 0;
        AttemptIndex < MaxSpawnLocationAttempts;
        ++AttemptIndex)
    {
        FVector SpawnLocation;
        if (!FindSafeSpawnLocation(SpawnLocation))
        {
            continue;
        }

        const FRotator SpawnRotation(
            0.0f,
            FMath::FRandRange(-180.0f, 180.0f),
            0.0f);

        ANF_Monster* SpawnedMonster =
            GetWorld()->SpawnActor<ANF_Monster>(
                MonsterClass,
                SpawnLocation,
                SpawnRotation,
                SpawnParameters);

        if (!SpawnedMonster)
        {
            // The point is on NavMesh but the monster capsule still overlaps
            // geometry or another actor. Try another candidate instead.
            continue;
        }

        SpawnedMonster->OnMonsterDied.AddUniqueDynamic(
            this,
            &ThisClass::HandleMonsterDied);

        AliveMonsters.Add(SpawnedMonster);

        return true;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("MonsterSpawner %s could not find a safe spawn location after %d attempts."),
        *GetName(), MaxSpawnLocationAttempts);
    return false;
}

bool ANF_MonsterSpawner::IsSpawnLocationSeparated(
    const FVector& CandidateLocation) const
{
    const float MinimumSeparationSquared =
        FMath::Square(MinimumSpawnSeparation);

    for (const TObjectPtr<ANF_Monster>& LivingMonster : AliveMonsters)
    {
        if (IsValid(LivingMonster)
            && FVector::DistSquared2D(
                CandidateLocation,
                LivingMonster->GetActorLocation()) < MinimumSeparationSquared)
        {
            return false;
        }
    }

    return true;
}

bool ANF_MonsterSpawner::FindSafeSpawnLocation(FVector& OutSpawnLocation) const
{
	UNavigationSystemV1* NavigationSystem =
        UNavigationSystemV1::GetCurrent(GetWorld());

    if (!NavigationSystem)
    {
        return false;
    }

    // A level designer places the spawner visually on the floor, but its actor
    // origin is not guaranteed to sit exactly on the generated NavMesh plane.
    // Project first so the reachable-point query always starts on navigation.
    FNavLocation ProjectedSpawnerLocation;
    const FVector ProjectionExtent(
        FMath::Max(SpawnRadius, 100.0f),
        FMath::Max(SpawnRadius, 100.0f),
        2000.0f);

    if (!NavigationSystem->ProjectPointToNavigation(
        GetActorLocation(),
        ProjectedSpawnerLocation,
        ProjectionExtent))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("MonsterSpawner %s has no NavMesh near its placed location."),
            *GetName());
        return false;
    }

    FNavLocation NavigationLocation;
    if (!NavigationSystem->GetRandomReachablePointInRadius(
        ProjectedSpawnerLocation.Location,
        SpawnRadius,
        NavigationLocation)
        || !IsSpawnLocationSeparated(NavigationLocation.Location))
    {
        return false;
    }

    OutSpawnLocation = NavigationLocation.Location;
    return true;
}

TSubclassOf<ANF_Monster> ANF_MonsterSpawner::GetRandomMonsterClass() const
{
    TArray<TSubclassOf<ANF_Monster>> ValidMonsterClasses;
    for (const TSubclassOf<ANF_Monster>& MonsterClass : MonsterClasses)
    {
        if (MonsterClass)
        {
            ValidMonsterClasses.Add(MonsterClass);
        }
    }

    if (ValidMonsterClasses.IsEmpty())
    {
        return nullptr;
    }

    return ValidMonsterClasses[FMath::RandRange(0, ValidMonsterClasses.Num() - 1)];
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
