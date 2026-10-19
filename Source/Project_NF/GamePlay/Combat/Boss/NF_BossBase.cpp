#include "GamePlay/Combat/Boss/NF_BossBase.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GamePlay/Combat/Boss/NF_BossAttributeSet.h"
#include "GamePlay/Combat/Boss/NF_BossRegistrySubsystem.h"
#include "Engine/DataTable.h"
#include "GamePlay/Combat/GameplayCue/NF_BossWarningMarker.h"
#include "GAS/Tags/NFGameplayCueTags.h"
#include "GamePlay/Combat/GameplayCue/NF_BossTelegraphMarker.h"
#include "Materials/MaterialInterface.h"
#include "Components/StateTreeComponent.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Player/Character/NF_Player.h"

ANF_BossBase::ANF_BossBase()
{
	BossDisplayName = NSLOCTEXT("NF_BossBase", "DefaultBossDisplayName", "Boss");
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	LegacyBossMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BossMesh"));
	LegacyBossMesh->SetupAttachment(SceneRoot);
	LegacyBossMesh->SetCollisionProfileName(TEXT("Pawn"));
	LegacyBossMesh->SetVisibility(false, true);
	LegacyBossMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	BossSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BossSkeletalMesh"));
	BossSkeletalMesh->SetupAttachment(SceneRoot);
	BossSkeletalMesh->SetCollisionProfileName(TEXT("Pawn"));

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	BossAttributeSet = CreateDefaultSubobject<UNF_BossAttributeSet>(TEXT("BossAttributeSet"));
	AbilitySystemComponent->AddAttributeSetSubobject(BossAttributeSet.Get());

	StateTreeComponent = CreateDefaultSubobject<UStateTreeComponent>(TEXT("BossStateTreeComponent"));
	StateTreeComponent->SetStartLogicAutomatically(false);

	TelegraphMarkerClass = ANF_BossTelegraphMarker::StaticClass();
}

UAbilitySystemComponent* ANF_BossBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

USkeletalMeshComponent* ANF_BossBase::GetBossSkeletalMesh() const
{
	return BossSkeletalMesh;
}

void ANF_BossBase::HandleGameplayCue(UObject* Self, const FGameplayTag GameplayCueTag,
	const EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters)
{
	if (GameplayCueTag == NFGameplayCueTags::GameplayCue_Boss_Telegraph
		&& EventType == EGameplayCueEvent::Executed)
	{
		UMaterialInterface* MaterialOverride = Cast<UMaterialInterface>(
			const_cast<UObject*>(Parameters.SourceObject.Get()));
		UMaterialInterface* MarkerMaterial = IsValid(MaterialOverride) ? MaterialOverride : TelegraphDecalMaterial.Get();
		if (GetNetMode() != NM_DedicatedServer && TelegraphMarkerClass && IsValid(MarkerMaterial))
		{
			ANF_BossTelegraphMarker* Marker = GetWorld()->SpawnActorDeferred<ANF_BossTelegraphMarker>(
				TelegraphMarkerClass, FTransform::Identity, this, nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (Marker)
			{
				const float TelegraphDuration = static_cast<float>(FMath::Max(Parameters.AbilityLevel, 1));
				const float MarkerLifetime = Parameters.NormalizedMagnitude > 0.0f
					? Parameters.NormalizedMagnitude : TelegraphDecalLifetime;
				Marker->InitializeMarker(Parameters.Location, Parameters.RawMagnitude,
					TelegraphDuration, MarkerLifetime, MarkerMaterial,
					TelegraphProgressParameterName, TelegraphProjectionDepth, TelegraphHeightOffset);
				Marker->FinishSpawning(FTransform::Identity);
			}
		}
		return;
	}
	if (GameplayCueTag == NFGameplayCueTags::GameplayCue_Boss_TargetWarning
		&& EventType == EGameplayCueEvent::Executed)
	{
		if (GetNetMode() != NM_DedicatedServer)
		{
			const float Lifetime = FMath::Max(static_cast<float>(Parameters.AbilityLevel), 0.25f);
			if (ANF_BossWarningMarker* Marker = GetWorld()->SpawnActor<ANF_BossWarningMarker>(
				ANF_BossWarningMarker::StaticClass(), Parameters.Location, FRotator::ZeroRotator))
			{
				Marker->SetLifeSpan(Lifetime);
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

	// The server must refresh skeletal bones to resolve animated socket transforms
	// (such as FirePoint) even though a dedicated server never renders the mesh.
	if (HasAuthority() && GetNetMode() == NM_DedicatedServer && IsValid(BossSkeletalMesh))
	{
		BossSkeletalMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	}

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	if (UWorld* MutableWorld = GetWorld())
	{
		if (UNF_BossRegistrySubsystem* BossRegistry = MutableWorld->GetSubsystem<UNF_BossRegistrySubsystem>())
		{
			BossRegistry->RegisterBoss(this);
		}
	}

	if (!HasAuthority())
	{
		return;
	}

	float ConfiguredMaxHealth = 10000.0f;
	bHasActiveCombatData = false;
	if (BossCombatDataTable)
	{
		static const FString ContextString(TEXT("ANF_BossBase::BeginPlay"));
		if (const FNF_BossCombatDataRow* CombatRow = BossCombatDataTable->FindRow<FNF_BossCombatDataRow>(
			BossCombatDataRowName, ContextString, false))
		{
			ActiveCombatData = *CombatRow;
			bHasActiveCombatData = true;
			ConfiguredMaxHealth = ActiveCombatData.MaxHealth;
			UE_LOG(LogTemp, Display,
				TEXT("[BossData] Loaded table=%s row=%s MaxHealth=%.1f AreaVolley=%.1f ThreatVolley=%.1f GroundZoneTick=%.1f Relocate=%.1f CenterBlast=%.1f LastGasp=%.1f"),
				*GetNameSafe(BossCombatDataTable), *BossCombatDataRowName.ToString(), ActiveCombatData.MaxHealth,
				ActiveCombatData.AreaVolleyDamage, ActiveCombatData.ThreatVolleyDamage,
				ActiveCombatData.GroundDamageZoneDamagePerTick, ActiveCombatData.RelocateDamage,
				ActiveCombatData.CenterBlastDamage, ActiveCombatData.LastGaspDamage);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[BossData] Row '%s' was not found in DataTable '%s'; using C++ defaults."),
				*BossCombatDataRowName.ToString(), *GetNameSafe(BossCombatDataTable));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[BossData] No combat DataTable assigned to %s; using C++ defaults."), *GetName());
	}
	if (BossAttributeSet)
	{
		BossAttributeSet->InitializeForBoss(ConfiguredMaxHealth);
	}

	for (const TSubclassOf<UGameplayAbility>& PatternAbility : CombatPatternAbilities)
	{
		if (PatternAbility && !AbilitySystemComponent->FindAbilitySpecFromClass(PatternAbility))
		{
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(PatternAbility, 1, INDEX_NONE, this));
		}
	}
	if (InitialPatternAbility && !CombatPatternAbilities.Contains(InitialPatternAbility))
	{
		CombatPatternAbilities.Insert(InitialPatternAbility, 0);
	}
	if (LastGaspAbility && !AbilitySystemComponent->FindAbilitySpecFromClass(LastGaspAbility))
	{
		AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(LastGaspAbility, 1, INDEX_NONE, this));
	}

	if (InitialPatternAbility)
	{
		FGameplayAbilitySpecHandle AbilityHandle;
		if (const FGameplayAbilitySpec* ExistingSpec = AbilitySystemComponent->FindAbilitySpecFromClass(InitialPatternAbility))
		{
			AbilityHandle = ExistingSpec->Handle;
		}
		else
		{
			AbilityHandle = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(InitialPatternAbility, 1, INDEX_NONE, this));
		}
		if (!BossStateTree && bActivateInitialPatternOnBeginPlay)
		{
			AbilitySystemComponent->TryActivateAbility(AbilityHandle);
		}
	}

	if (BossStateTree)
	{
		StateTreeComponent->SetStateTree(BossStateTree);
		StateTreeComponent->StartLogic();
	}
}

void ANF_BossBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UNF_BossRegistrySubsystem* BossRegistry = World->GetSubsystem<UNF_BossRegistrySubsystem>())
		{
			BossRegistry->UnregisterBoss(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

bool ANF_BossBase::ActivateNextCombatPattern()
{
	if (!HasAuthority() || bIsDefeated || CombatPatternAbilities.IsEmpty() || !AbilitySystemComponent)
	{
		return false;
	}

	for (int32 Attempts = 0; Attempts < CombatPatternAbilities.Num(); ++Attempts)
	{
		const int32 Index = NextCombatPatternIndex++ % CombatPatternAbilities.Num();
		const TSubclassOf<UGameplayAbility> AbilityClass = CombatPatternAbilities[Index];
		if (!AbilityClass)
		{
			continue;
		}
		if (AbilitySystemComponent->TryActivateAbilityByClass(AbilityClass))
		{
			UE_LOG(LogTemp, Display, TEXT("[BossStateTree] Started pattern %s on %s"),
				*GetNameSafe(AbilityClass), *GetName());
			return true;
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("[BossStateTree] No configured combat pattern could activate on %s"), *GetName());
	return false;
}

bool ANF_BossBase::IsCombatPatternActive() const
{
	if (!AbilitySystemComponent)
	{
		return false;
	}
	for (const TSubclassOf<UGameplayAbility>& AbilityClass : CombatPatternAbilities)
	{
		const FGameplayAbilitySpec* Spec = AbilityClass ? AbilitySystemComponent->FindAbilitySpecFromClass(AbilityClass) : nullptr;
		if (Spec && Spec->IsActive())
		{
			return true;
		}
	}
	return false;
}

bool ANF_BossBase::ActivateAbilityClass(const TSubclassOf<UGameplayAbility> AbilityClass,
	const bool bAllowWhenDefeated)
{
	return HasAuthority() && (!bIsDefeated || bAllowWhenDefeated) && AbilityClass
		&& AbilitySystemComponent && AbilitySystemComponent->TryActivateAbilityByClass(AbilityClass);
}

bool ANF_BossBase::IsAbilityClassActive(const TSubclassOf<UGameplayAbility> AbilityClass) const
{
	if (!AbilityClass || !AbilitySystemComponent)
	{
		return false;
	}
	const FGameplayAbilitySpec* Spec = AbilitySystemComponent->FindAbilitySpecFromClass(AbilityClass);
	return Spec && Spec->IsActive();
}

ANF_Player* ANF_BossBase::GetHighestDamageTarget(const float MaxRange) const
{
	ANF_Player* BestPlayer = nullptr;
	float BestDamage = -1.0f;
	const float MaxRangeSquared = FMath::Square(FMath::Max(MaxRange, 0.0f));
	for (const TPair<TWeakObjectPtr<ANF_Player>, float>& Entry : DamageContribution)
	{
		ANF_Player* Player = Entry.Key.Get();
		if (!IsValid(Player) || FVector::DistSquared(GetActorLocation(), Player->GetActorLocation()) > MaxRangeSquared)
		{
			continue;
		}
		if (Entry.Value > BestDamage)
		{
			BestDamage = Entry.Value;
			BestPlayer = Player;
		}
	}
	return BestPlayer;
}

float ANF_BossBase::GetPatternDamage(const ENF_BossPatternId PatternId, const float FallbackDamage) const
{
	if (!bHasActiveCombatData)
	{
		return FMath::Max(FallbackDamage, 0.0f);
	}

	float ConfiguredDamage = FallbackDamage;
	switch (PatternId)
	{
	case ENF_BossPatternId::AreaVolley:
		ConfiguredDamage = ActiveCombatData.AreaVolleyDamage;
		break;
	case ENF_BossPatternId::ThreatVolley:
		ConfiguredDamage = ActiveCombatData.ThreatVolleyDamage;
		break;
	case ENF_BossPatternId::GroundDamageZone:
		ConfiguredDamage = ActiveCombatData.GroundDamageZoneDamagePerTick;
		break;
	case ENF_BossPatternId::Relocate:
		ConfiguredDamage = ActiveCombatData.RelocateDamage;
		break;
	case ENF_BossPatternId::CenterBlast:
		ConfiguredDamage = ActiveCombatData.CenterBlastDamage;
		break;
	case ENF_BossPatternId::LastGasp:
		ConfiguredDamage = ActiveCombatData.LastGaspDamage;
		break;
	default:
		break;
	}
	return FMath::Max(ConfiguredDamage, 0.0f);
}

bool ANF_BossBase::ActivateLastGasp()
{
	return bIsDefeated && ActivateAbilityClass(LastGaspAbility, true);
}

bool ANF_BossBase::IsLastGaspActive() const
{
	return IsAbilityClassActive(LastGaspAbility);
}

void ANF_BossBase::RecordDamage(ANF_Player* Player, const float DamageAmount)
{
	if (HasAuthority() && IsValid(Player) && DamageAmount > 0.0f)
	{
		DamageContribution.FindOrAdd(Player) += DamageAmount;
	}
}

void ANF_BossBase::HandleHealthUpdated(const float NewHealth)
{
	if (HasAuthority() && NewHealth <= 0.0f && !bDeathSequenceStarted)
	{
		BeginDeathSequence();
	}
}

void ANF_BossBase::BeginDeathSequence()
{
	if (!HasAuthority() || bDeathSequenceStarted)
	{
		return;
	}
	bDeathSequenceStarted = true;
	bIsDefeated = true;
	OnRep_IsDefeated();
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->CancelAllAbilities();
	}
	if (BossStateTree && StateTreeComponent && StateTreeComponent->IsRunning())
	{
		// The StateTree task observes bIsDefeated and runs the last-gasp ability.
		return;
	}
	if (ActivateLastGasp())
	{
		return;
	}
	CompleteDeathSequence();
}

void ANF_BossBase::CompleteDeathSequence()
{
	if (!HasAuthority())
	{
		return;
	}
	if (bDestroyWhenDeathSequenceCompletes)
	{
		Destroy();
	}
}

void ANF_BossBase::SetSubmerged(const bool bNewSubmerged)
{
	if (!HasAuthority())
	{
		return;
	}
	bIsSubmerged = bNewSubmerged;
	ApplySubmergedPresentation();
}

void ANF_BossBase::SetFlying(const bool bNewFlying)
{
	if (HasAuthority())
	{
		bIsFlying = bNewFlying;
	}
}

void ANF_BossBase::OnRep_IsDefeated()
{
	if (bIsDefeated)
	{
		SetActorEnableCollision(false);
	}
}

void ANF_BossBase::OnRep_IsSubmerged()
{
	ApplySubmergedPresentation();
}

void ANF_BossBase::ApplySubmergedPresentation()
{
	if (BossSkeletalMesh)
	{
		BossSkeletalMesh->SetVisibility(!bIsSubmerged, true);
		BossSkeletalMesh->SetCollisionEnabled(bIsSubmerged ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	}
}

void ANF_BossBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANF_BossBase, bIsDefeated);
	DOREPLIFETIME(ANF_BossBase, bIsSubmerged);
	DOREPLIFETIME(ANF_BossBase, bIsFlying);
}
