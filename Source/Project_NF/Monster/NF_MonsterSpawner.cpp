#include "Monster/NF_MonsterSpawner.h"

#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
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

    PendingInitialSpawnCount = FMath::Clamp(InitialSpawnCount, 0, FMath::Max(MaxAliveCount, 0));
    SpawnInitialMonsters();
    if (PendingInitialSpawnCount > 0)
    {
        UE_LOG(LogTemp, Display, TEXT("MonsterSpawner %s waiting for navigation / ground collision (%d remaining)."),
            *GetName(), PendingInitialSpawnCount);
    }
}

void ANF_MonsterSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearTimer(InitialSpawnRetryTimerHandle);
    GetWorldTimerManager().ClearTimer(RespawnTimerHandle);
    Super::EndPlay(EndPlayReason);
}

void ANF_MonsterSpawner::SpawnInitialMonsters()
{
    if (!HasAuthority()) { return; }
    const int32 SpawnCount = PendingInitialSpawnCount;

    for (int32 Index = 0; Index < SpawnCount; ++Index)
    {
        if (SpawnMonster())
        {
            --PendingInitialSpawnCount;
        }
    }

    // Level instances can register their floor collision after the persistent
    // level's BeginPlay, particularly on dedicated servers. Retry only missing
    // initial monsters; do not turn this into an unconditional population refill.
    if (PendingInitialSpawnCount > 0)
    {
        GetWorldTimerManager().SetTimer(InitialSpawnRetryTimerHandle, this,
            &ThisClass::SpawnInitialMonsters, 0.5f, false);
    }
}

bool ANF_MonsterSpawner::SpawnMonster()
{
    if (!HasAuthority() || MonsterClasses.IsEmpty()
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
        if (!FindSafeSpawnLocation(MonsterClass, SpawnLocation))
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
            continue;
        }
        
        SpawnedMonster->SetReplicates(true);
        SpawnedMonster->SetReplicateMovement(true);
        SpawnedMonster->SetNetDormancy(DORM_Awake);
        SpawnedMonster->ForceNetUpdate();

        SpawnedMonster->OnMonsterDied.AddUniqueDynamic(
            this,
            &ThisClass::HandleMonsterDied);

        AliveMonsters.Add(SpawnedMonster);
        
        return true;
    }

    UE_LOG(LogTemp, Verbose,
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

bool ANF_MonsterSpawner::FindSafeSpawnLocation(
    TSubclassOf<ANF_Monster> MonsterClass, FVector& OutSpawnLocation) const
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
        UE_LOG(LogTemp, Verbose,
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

    const ANF_Monster* MonsterDefaults = MonsterClass.GetDefaultObject();
    const UCapsuleComponent* Capsule = MonsterDefaults ? MonsterDefaults->GetCapsuleComponent() : nullptr;
    const UCharacterMovementComponent* Movement = MonsterDefaults ? MonsterDefaults->GetCharacterMovement() : nullptr;
    if (!Capsule || !Movement)
    {
        return false;
    }

    // A baked NavMesh may already exist while the streamed floor does not.
    // Require a real, walkable blocking surface using the capsule's collision
    // responses. NavMesh locations are foot positions, not capsule centers.
    const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
    const float GroundTolerance = FMath::Max(Movement->MaxStepHeight, 50.0f);
    const FVector TraceStart = NavigationLocation.Location + FVector(0, 0, GroundTolerance);
    const FVector TraceEnd = NavigationLocation.Location - FVector(0, 0, GroundTolerance);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MonsterSpawnGround), false, this);
    FHitResult GroundHit;
    if (!GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd,
        Capsule->GetCollisionObjectType(), QueryParams,
        FCollisionResponseParams(Capsule->GetCollisionResponseToChannels()))
        || !Movement->IsWalkable(GroundHit))
    {
        return false;
    }

    OutSpawnLocation = GroundHit.ImpactPoint + FVector(0, 0, HalfHeight + 2.0f);
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
