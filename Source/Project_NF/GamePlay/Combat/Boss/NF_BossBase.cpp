#include "GamePlay/Combat/Boss/NF_BossBase.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GamePlay/Combat/Boss/NF_BossAttributeSet.h"
#include "GamePlay/Combat/Boss/NF_BossRegistrySubsystem.h"
#include "Core/NF_GameMode.h"
#include "Data/Struct/NF_BossDefeatResult.h"
#include "Engine/DataTable.h"
#include "GamePlay/Combat/GameplayCue/NF_BossWarningMarker.h"
#include "GAS/Tags/NFGameplayCueTags.h"
#include "GamePlay/Combat/GameplayCue/NF_BossTelegraphMarker.h"
#include "Materials/MaterialInterface.h"
#include "Components/StateTreeComponent.h"
#include "Engine/World.h"
#include "StateTree.h"
#include "Engine/LocalPlayer.h"
#include "INotifyFieldValueChanged.h"
#include "MVVMSubsystem.h"
#include "View/MVVMView.h"
#include "Net/UnrealNetwork.h"
#include "Player/Character/NF_Player.h"
#include "Player/State/NF_PlayerState.h"
#include "Project_NF.h"
#include "UI/SubSystem/NF_ViewModelSubsystem.h"
#include "UI/ViewModel/Status/NF_MonsterStatusViewModel.h"
#include "Blueprint/UserWidget.h"

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

	BossHealthWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("BossHealthWidget"));
	BossHealthWidgetComponent->SetupAttachment(BossSkeletalMesh);
	BossHealthWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 500.0f));
	BossHealthWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	BossHealthWidgetComponent->SetDrawSize(FVector2D(500.0f, 100.0f));
	BossHealthWidgetComponent->SetPivot(FVector2D(0.5f, 0.5f));
	BossHealthWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BossHealthWidgetComponent->SetIsReplicated(false);

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

void ANF_BossBase::InitializeBossHealthWidgetForLocalPlayer()
{
	if (GetNetMode() == NM_DedicatedServer || !IsValid(BossHealthWidgetComponent))
	{
		return;
	}

	if (!BossHealthWidgetComponent->GetWidgetClass())
	{
		UE_LOG(LogProject_NF, Verbose,
			TEXT("[BossHealthUI] No Widget Class assigned on %s."), *GetNameSafe(this));
		GetWorldTimerManager().ClearTimer(BossHealthWidgetInitTimer);
		return;
	}

	BossHealthWidgetComponent->InitWidget();
	UUserWidget* Widget = BossHealthWidgetComponent->GetUserWidgetObject();
	ULocalPlayer* LocalPlayer = GetWorld() ? GetWorld()->GetFirstLocalPlayerFromController() : nullptr;
	UNF_ViewModelSubsystem* ViewModelSubsystem = IsValid(LocalPlayer)
		? LocalPlayer->GetSubsystem<UNF_ViewModelSubsystem>()
		: nullptr;
	UNF_MonsterStatusViewModel* MonsterStatusViewModel = IsValid(ViewModelSubsystem)
		? ViewModelSubsystem->GetMonsterStatusViewModel()
		: nullptr;

	if (!IsValid(Widget) || !IsValid(MonsterStatusViewModel))
	{
		// The replicated boss can arrive before the local PlayerController and its LocalPlayer subsystem.
		if (!GetWorldTimerManager().IsTimerActive(BossHealthWidgetInitTimer))
		{
			GetWorldTimerManager().SetTimer(BossHealthWidgetInitTimer, this,
				&ThisClass::InitializeBossHealthWidgetForLocalPlayer, 0.25f, true);
		}
		return;
	}

	UMVVMView* View = UMVVMSubsystem::GetViewFromUserWidget(Widget);
	INotifyFieldValueChanged* ViewModelInterface = Cast<INotifyFieldValueChanged>(MonsterStatusViewModel);
	if (!View || !ViewModelInterface)
	{
		UE_LOG(LogProject_NF, Error,
			TEXT("[BossHealthUI] Widget %s has no initialized MVVM view or compatible ViewModel."),
			*GetNameSafe(Widget));
		GetWorldTimerManager().ClearTimer(BossHealthWidgetInitTimer);
		return;
	}

	TScriptInterface<INotifyFieldValueChanged> ViewModel;
	ViewModel.SetObject(MonsterStatusViewModel);
	ViewModel.SetInterface(ViewModelInterface);
	if (!View->SetViewModelByClass(ViewModel))
	{
		UE_LOG(LogProject_NF, Error,
			TEXT("[BossHealthUI] Could not assign the existing monster status ViewModel to %s. Set its ViewModel creation type to Manual."),
			*GetNameSafe(Widget));
		GetWorldTimerManager().ClearTimer(BossHealthWidgetInitTimer);
		return;
	}

	GetWorldTimerManager().ClearTimer(BossHealthWidgetInitTimer);
	UE_LOG(LogProject_NF, Display,
		TEXT("[BossHealthUI] Bound widget %s to local boss status ViewModel for %s."),
		*GetNameSafe(Widget), *GetNameSafe(this));
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
	InitializeBossHealthWidgetForLocalPlayer();

	if (!HasAuthority())
	{
		return;
	}

	float ConfiguredMaxHealth = 10000.0f;
	bHasActiveCombatData = false;
	if (!BossCombatDataTable)
	{
		BossCombatDataTable = LoadObject<UDataTable>(nullptr,
			TEXT("/Game/ProjectNF/Persnal/Myungwoo/Data/Boss/DT_BossCombat.DT_BossCombat"));
	}
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
		if (!AbilitySystemComponent->FindAbilitySpecFromClass(InitialPatternAbility))
		{
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(InitialPatternAbility, 1, INDEX_NONE, this));
		}
	}

	if (!BossStateTree)
	{
		BossStateTree = LoadObject<UStateTree>(nullptr,
			TEXT("/Game/ProjectNF/Persnal/Myungwoo/Gameplay/Boss/ST_BossPatternCycle.ST_BossPatternCycle"));
	}

	UE_LOG(LogTemp, Display,
		TEXT("[BossStateTreeDiag] Setup Boss=%s Tree=%s PatternAbilityCount=%d Awake=%d ASC=%s"),
		*GetName(), *GetPathNameSafe(BossStateTree), CombatPatternAbilities.Num(), bIsAwake ? 1 : 0,
		*GetNameSafe(AbilitySystemComponent));
	for (int32 AbilityIndex = 0; AbilityIndex < CombatPatternAbilities.Num(); ++AbilityIndex)
	{
		const TSubclassOf<UGameplayAbility> AbilityClass = CombatPatternAbilities[AbilityIndex];
		const bool bGranted = AbilityClass && AbilitySystemComponent
			&& AbilitySystemComponent->FindAbilitySpecFromClass(AbilityClass) != nullptr;
		UE_LOG(LogTemp, Display,
			TEXT("[BossStateTreeDiag] ConfiguredPattern[%d]=%s Granted=%d"),
			AbilityIndex, *GetNameSafe(AbilityClass), bGranted ? 1 : 0);
	}

	if (BossStateTree && StateTreeComponent)
	{
		UE_LOG(LogTemp, Display, TEXT("[BossStateTreeDiag] Assigning and starting tree %s on %s"),
			*GetPathNameSafe(BossStateTree), *GetName());
		StateTreeComponent->SetStateTree(BossStateTree);
		StateTreeComponent->StartLogic();
		UE_LOG(LogTemp, Display, TEXT("[BossStateTreeDiag] StartLogic returned; Running=%d Boss=%s"),
			StateTreeComponent->IsRunning() ? 1 : 0, *GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("[BossStateTreeDiag] Cannot start patterns: Tree=%s ComponentValid=%d Boss=%s"),
			*GetPathNameSafe(BossStateTree), IsValid(StateTreeComponent) ? 1 : 0, *GetName());
	}
}

void ANF_BossBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(BossHealthWidgetInitTimer);
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

bool ANF_BossBase::WakeBoss()
{
	if (!HasAuthority() || bIsDefeated || bIsAwake)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[BossStateTreeDiag] WakeBoss rejected Boss=%s Authority=%d Defeated=%d AlreadyAwake=%d Tree=%s"),
			*GetName(), HasAuthority() ? 1 : 0, bIsDefeated ? 1 : 0, bIsAwake ? 1 : 0,
			*GetPathNameSafe(BossStateTree));
		return false;
	}

	bIsAwake = true;
	UE_LOG(LogTemp, Display,
		TEXT("[BossStateTreeDiag] WakeBoss succeeded Boss=%s TreeAssignedAtCall=%d Tree=%s StateTreeRunning=%d PatternAbilityCount=%d"),
		*GetName(), BossStateTree ? 1 : 0, *GetPathNameSafe(BossStateTree),
		StateTreeComponent && StateTreeComponent->IsRunning() ? 1 : 0, CombatPatternAbilities.Num());

	// Without an assigned StateTree, retain the initial-ability fallback but start it only on wake.
	if (!BossStateTree && bActivateInitialPatternOnBeginPlay && InitialPatternAbility && AbilitySystemComponent)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[BossStateTreeDiag] No tree assigned during WakeBoss; trying initial GA fallback Ability=%s"),
			*GetNameSafe(InitialPatternAbility.Get()));
		ActivateAbilityClass(InitialPatternAbility);
	}

	return true;
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
	const UClass* AbilityUClass = AbilityClass.Get();
	const FGameplayAbilitySpec* AbilitySpec = AbilityUClass && AbilitySystemComponent
		? AbilitySystemComponent->FindAbilitySpecFromClass(AbilityClass)
		: nullptr;

	if (!HasAuthority() || (bIsDefeated && !bAllowWhenDefeated) || !AbilityUClass || !AbilitySystemComponent)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[BossGADiag] Activation blocked Boss=%s Ability=%s Authority=%d Defeated=%d AllowWhenDefeated=%d ASC=%s"),
			*GetName(), *GetNameSafe(AbilityUClass), HasAuthority() ? 1 : 0, bIsDefeated ? 1 : 0,
			bAllowWhenDefeated ? 1 : 0, *GetNameSafe(AbilitySystemComponent));
		return false;
	}

	if (!AbilitySpec)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[BossGADiag] Activation failed: ability is not granted. Boss=%s Ability=%s"),
			*GetName(), *GetNameSafe(AbilityUClass));
		return false;
	}

	const bool bActivated = AbilitySystemComponent->TryActivateAbilityByClass(AbilityClass);
	if (bActivated)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[BossGADiag] TryActivateAbilityByClass succeeded Boss=%s Ability=%s Active=%d"),
			*GetName(), *GetNameSafe(AbilityUClass), AbilitySpec->IsActive() ? 1 : 0);
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[BossGADiag] TryActivateAbilityByClass failed Boss=%s Ability=%s; check ability tags, costs, and activation requirements"),
			*GetName(), *GetNameSafe(AbilityUClass));
	}
	return bActivated;
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
	for (const TPair<TWeakObjectPtr<ANF_PlayerState>, float>& Entry : DamageContribution)
	{
		ANF_PlayerState* PlayerState = Entry.Key.Get();
		ANF_Player* Player = IsValid(PlayerState) ? Cast<ANF_Player>(PlayerState->GetPawn()) : nullptr;
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

void ANF_BossBase::RecordDamage(ANF_PlayerState* PlayerState, const float DamageAmount,
	const bool bIsFatalDamage)
{
	if (!HasAuthority() || DamageAmount <= 0.0f)
	{
		return;
	}

	if (IsValid(PlayerState))
	{
		DamageContribution.FindOrAdd(PlayerState) += DamageAmount;
	}
	if (bIsFatalDamage)
	{
		LastHitPlayerState = PlayerState;
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
	NotifyCoreBossDefeated();
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

void ANF_BossBase::NotifyCoreBossDefeated()
{
	if (!HasAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();
	ANF_GameMode* GameMode = IsValid(World) ? World->GetAuthGameMode<ANF_GameMode>() : nullptr;
	if (!IsValid(GameMode))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[BossDefeat] Could not notify Core: authoritative ANF_GameMode is unavailable. Boss=%s"),
			*GetNameSafe(this));
		return;
	}

	FNF_BossDefeatResult DefeatResult;
	DefeatResult.DefeatedBoss = this;
	DefeatResult.LastHitPlayerState = LastHitPlayerState.Get();
	for (const TPair<TWeakObjectPtr<ANF_PlayerState>, float>& Entry : DamageContribution)
	{
		ANF_PlayerState* PlayerState = Entry.Key.Get();
		if (!IsValid(PlayerState) || Entry.Value <= 0.0f)
		{
			continue;
		}

		FNF_BossDamageContribution& Contribution = DefeatResult.Contributions.AddDefaulted_GetRef();
		Contribution.PlayerState = PlayerState;
		Contribution.EffectiveDamage = Entry.Value;
	}

	const bool bAccepted = GameMode->NotifyBossDefeated(DefeatResult);
	if (!bAccepted)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[BossDefeat] Core rejected defeat result; boss death continues without retry. Boss=%s LastHit=%s ContributionCount=%d"),
			*GetNameSafe(this), *GetNameSafe(DefeatResult.LastHitPlayerState.Get()),
			DefeatResult.Contributions.Num());
	}
	else
	{
		UE_LOG(LogTemp, Display,
			TEXT("[BossDefeat] Core accepted defeat result. Boss=%s LastHit=%s ContributionCount=%d"),
			*GetNameSafe(this), *GetNameSafe(DefeatResult.LastHitPlayerState.Get()),
			DefeatResult.Contributions.Num());
	}
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
