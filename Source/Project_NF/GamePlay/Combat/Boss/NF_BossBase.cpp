#include "GamePlay/Combat/Boss/NF_BossBase.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Components/StaticMeshComponent.h"
#include "GAS/Tags/NFGameplayCueTags.h"
#include "GamePlay/Combat/GameplayCue/NF_BossTelegraphMarker.h"
#include "Engine/World.h"

ANF_BossBase::ANF_BossBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	BossMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BossMesh"));
	BossMesh->SetupAttachment(SceneRoot);
	BossMesh->SetCollisionProfileName(TEXT("Pawn"));

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	TelegraphMarkerClass = ANF_BossTelegraphMarker::StaticClass();
}

UAbilitySystemComponent* ANF_BossBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ANF_BossBase::HandleGameplayCue(UObject* Self, const FGameplayTag GameplayCueTag,
	const EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters)
{
	if (GameplayCueTag == NFGameplayCueTags::GameplayCue_Boss_Telegraph
		&& EventType == EGameplayCueEvent::Executed)
	{
		if (GetNetMode() != NM_DedicatedServer && TelegraphMarkerClass && TelegraphDecalMaterial)
		{
			ANF_BossTelegraphMarker* Marker = GetWorld()->SpawnActorDeferred<ANF_BossTelegraphMarker>(
				TelegraphMarkerClass, FTransform::Identity, this, nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (Marker)
			{
				Marker->InitializeMarker(Parameters.Location, Parameters.RawMagnitude,
					static_cast<float>(FMath::Max(Parameters.AbilityLevel, 1)), TelegraphDecalMaterial,
					TelegraphProgressParameterName, TelegraphProjectionDepth, TelegraphHeightOffset);
				Marker->FinishSpawning(FTransform::Identity);
			}
		}
		return;
	}

	IGameplayCueInterface::HandleGameplayCue(Self, GameplayCueTag, EventType, Parameters);
}

void ANF_BossBase::BeginPlay()
{
	Super::BeginPlay();

	const UWorld* World = GetWorld();
	UE_LOG(LogTemp, Warning,
		TEXT("[BossDiag] BeginPlay Actor=%s Map=%s Authority=%d NetMode=%d Replicates=%d LocalRole=%d"),
		*GetPathName(),
		World ? *World->GetMapName() : TEXT("<NoWorld>"),
		HasAuthority() ? 1 : 0,
		World ? static_cast<int32>(World->GetNetMode()) : INDEX_NONE,
		GetIsReplicated() ? 1 : 0,
		static_cast<int32>(GetLocalRole()));

	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	if (!HasAuthority() || !InitialPatternAbility)
	{
		return;
	}

	const FGameplayAbilitySpecHandle AbilityHandle = AbilitySystemComponent->GiveAbility(
		FGameplayAbilitySpec(InitialPatternAbility, 1, INDEX_NONE, this));
	if (bActivateInitialPatternOnBeginPlay)
	{
		AbilitySystemComponent->TryActivateAbility(AbilityHandle);
	}
}
