#include "Monster/NF_Monster.h"
#include "GameplayEffect.h"
#include "GAS/Foundation/NF_AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayAbilitySpec.h"
#include "GAS/Attributes/NF_MonsterAttributeSet.h"
#include "AI/NF_MonsterAIController.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GAS/Tags/NFTag_Monster.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Monster/NF_MonsterItemDrop.h"
#include "Monster/NF_MonsterSpawner.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "UI/Widget/NF_MonsterStatusWidget.h"
#include "Player/State/NF_PlayerState.h"
#include "Player/Character/NF_PlayerLevelAttribute.h"
#include "GAS/Tags/NFTag_BossCombat.h"

#include "Player/Character/NF_PlayerStatusAttribute.h"

ANF_Monster::ANF_Monster()
{
	MonsterDisplayName = NSLOCTEXT("NF_Monster", "DefaultMonsterDisplayName", "Monster");
	bReplicates = true;
	SetReplicateMovement(true);
	
	//2026-10-23 데디환경 스폰
	bAlwaysRelevant = true;

	SetNetUpdateFrequency(30.0f);
	SetMinNetUpdateFrequency(10.0f);

	// 2026-10-22 몬스터 Mesh 의 콜리전 설정 수정 요청
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GetMesh()->SetCollisionObjectType(ECC_Pawn); //카메라간섭이슈해결
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	
	MonsterASC =
	CreateDefaultSubobject<UNF_AbilitySystemComponent>(
		TEXT("MonsterASC"));

	MonsterASC->SetIsReplicated(true);
	MonsterASC->SetReplicationMode(
		EGameplayEffectReplicationMode::Minimal);

	MonsterAttributeSet =
		CreateDefaultSubobject<UNF_MonsterAttributeSet>(
			TEXT("MonsterAttributeSet"));

	MonsterStatusWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("MonsterStatusWidget"));
	MonsterStatusWidgetComponent->SetupAttachment(GetRootComponent());
	MonsterStatusWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 130.0f));
	MonsterStatusWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	MonsterStatusWidgetComponent->SetDrawSize(FVector2D(220.0f, 55.0f));
	MonsterStatusWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MonsterStatusWidgetComponent->SetIsReplicated(false);
}

UAbilitySystemComponent* ANF_Monster::GetAbilitySystemComponent() const
{
	return MonsterASC.Get();
}

void ANF_Monster::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() != NM_DedicatedServer)
	{
		if (MonsterStatusWidgetClass)
		{
			MonsterStatusWidgetComponent->SetWidgetClass(MonsterStatusWidgetClass);
		}
		else if (!MonsterStatusWidgetComponent->GetWidgetClass())
		{
			MonsterStatusWidgetComponent->SetWidgetClass(UNF_MonsterStatusWidget::StaticClass());
		}
		MonsterStatusWidgetComponent->InitWidget();
		if (UNF_MonsterStatusWidget* StatusWidget = Cast<UNF_MonsterStatusWidget>(MonsterStatusWidgetComponent->GetUserWidgetObject()))
		{
			StatusWidget->ObserveMonster(this);
		}
		MonsterStatusWidgetComponent->SetVisibility(false);
		GetWorldTimerManager().SetTimer(StatusVisibilityTimerHandle, this,
			&ThisClass::UpdateLocalStatusVisibility, 0.2f, true);
	}
	
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[WorldCheck] Map=%s | NetMode=%d | Monster=%s"),
		*GetWorld()->GetMapName(),
		static_cast<int32>(GetNetMode()),
		*GetName());
	
	MonsterASC->InitAbilityActorInfo(this, this);
	
	//Attribute 변경 감지 연결
	MonsterASC->GetGameplayAttributeValueChangeDelegate(
	UNF_MonsterAttributeSet::GetMoveSpeedAttribute())
	.AddUObject(
		this,
		&ANF_Monster::HandleMoveSpeedChanged);
	
	MonsterAttributeSet->OnMonsterDamaged.AddUniqueDynamic(
		this,
		&ThisClass::HandleMonsterDamaged);
	
	MonsterAttributeSet->OnMonsterOutOfHealth.AddUniqueDynamic(
	this,
	&ThisClass::HandleMonsterOutOfHealth);

	if (HasAuthority() && DefaultAttributesEffect)
	{
		FGameplayEffectContextHandle EffectContext =
			MonsterASC->MakeEffectContext();

		EffectContext.AddSourceObject(this);

		const FGameplayEffectSpecHandle EffectSpecHandle =
			MonsterASC->MakeOutgoingSpec(
				DefaultAttributesEffect,
				1.0f,
				EffectContext);

		if (EffectSpecHandle.IsValid())
		{
			MonsterASC->ApplyGameplayEffectSpecToSelf(
				*EffectSpecHandle.Data.Get());
			
			const float MaxHealth = MonsterAttributeSet->GetMaxHealth();
			if (MaxHealth > 0.0f && MonsterAttributeSet->GetHealth() <= 0.0f)
			{
				MonsterAttributeSet->SetHealth(MaxHealth);
			}
			
			UE_LOG(
	LogTemp,
	Warning,
	TEXT("Monster Initial Stats | Monster=%s | DefaultGE=%s | Health=%.1f | MaxHealth=%.1f"),
	*GetName(),
	*GetNameSafe(DefaultAttributesEffect),
	MonsterAttributeSet->GetHealth(),
	MonsterAttributeSet->GetMaxHealth());
			
		}
	}
	
	SetMonsterMoveSpeed(MonsterAttributeSet->GetMoveSpeed());
	
	if (HasAuthority())
	{
		for (const TSubclassOf<UGameplayAbility>& AbilityClass
			: StartupAbilities)
		{
			if (AbilityClass)
			{
				MonsterASC->GiveAbility(
					FGameplayAbilitySpec(
						AbilityClass,
						1,
						INDEX_NONE,
						this));
				/*
				UE_LOG(
				LogTemp,
				Display,
				TEXT("[%s] Startup Ability Granted: %s"),
				*GetName(),
				*GetNameSafe(AbilityClass));
				*/
			}
		}
	}
	
	if (GetNetMode() == NM_Client)
	{
		USkeletalMeshComponent* MonsterMesh = GetMesh();
		if (IsValid(MonsterMesh))
		{
			MonsterMesh->SetVisibility(true, true);
			MonsterMesh->SetHiddenInGame(false, true);
			MonsterMesh->SetOwnerNoSee(false);
			MonsterMesh->SetOnlyOwnerSee(false);

			// Bounds/LOD 또는 Main Pass 설정 문제인지 분리하기 위한 임시 강제값
			MonsterMesh->SetRenderInMainPass(true);
			MonsterMesh->SetBoundsScale(10.0f);
			MonsterMesh->SetForcedLOD(1);
			MonsterMesh->MarkRenderStateDirty();

			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[MonsterVisual] Registered=%d RenderMainPass=%d Bounds=%s"),
				MonsterMesh->IsRegistered(),
				MonsterMesh->bRenderInMainPass,
				*MonsterMesh->Bounds.GetBox().ToString());
		}
	}
	
	HomeLocation = GetActorLocation();
}

void ANF_Monster::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANF_Monster, MonsterDisplayName);
}

void ANF_Monster::OnRep_MonsterDisplayName()
{
	if (IsValid(MonsterStatusWidgetComponent))
	{
		if (UNF_MonsterStatusWidget* StatusWidget = Cast<UNF_MonsterStatusWidget>(MonsterStatusWidgetComponent->GetUserWidgetObject()))
		{
			StatusWidget->RefreshMonsterName();
		}
	}
}

void ANF_Monster::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(StatusVisibilityTimerHandle);
	if (IsValid(MonsterStatusWidgetComponent))
	{
		if (UNF_MonsterStatusWidget* StatusWidget = Cast<UNF_MonsterStatusWidget>(MonsterStatusWidgetComponent->GetUserWidgetObject()))
		{
			StatusWidget->ObserveMonster(nullptr);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void ANF_Monster::UpdateLocalStatusVisibility()
{
	if (!IsValid(MonsterStatusWidgetComponent)) return;
	APlayerController* LocalController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	const APawn* LocalPawn = IsValid(LocalController) && LocalController->IsLocalController()
		? LocalController->GetPawn() : nullptr;
	const bool bNearPlayer = IsValid(LocalPawn) &&
		FVector::DistSquared(LocalPawn->GetActorLocation(), GetActorLocation()) <= FMath::Square(StatusVisibleDistance);
	const bool bRecentlyAttacked = GetWorld()->GetTimeSeconds() < StatusRevealedUntil;
	const bool bAlive = IsValid(MonsterAttributeSet) && MonsterAttributeSet->GetHealth() > 0.0f;
	MonsterStatusWidgetComponent->SetVisibility(bAlive && (bNearPlayer || bRecentlyAttacked));
}

void ANF_Monster::MulticastRevealStatusToAttacker_Implementation(APlayerState* Attacker)
{
	if (GetNetMode() == NM_DedicatedServer || !IsValid(Attacker)) return;
	APlayerController* LocalController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!IsValid(LocalController) || !LocalController->IsLocalController() || LocalController->PlayerState != Attacker) return;
	StatusRevealedUntil = GetWorld()->GetTimeSeconds() + StatusDamageVisibleDuration;
	UpdateLocalStatusVisibility();
}

void ANF_Monster::ApplyBasicAttackHit()
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("Attack failed: Not Authority"));
		return;
	}

	if (!MonsterASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("Attack failed: MonsterASC is null"));
		return;
	}

	if (!MonsterAttributeSet)
	{
		UE_LOG(LogTemp, Warning, TEXT("Attack failed: MonsterAttributeSet is null"));
		return;
	}

	if (!BasicAttackDamageEffect)
	{
		UE_LOG(LogTemp, Warning, TEXT("Attack failed: BasicAttackDamageEffect is null"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Attack: Server validation passed"));

    ANF_MonsterAIController* MonsterAIController =
        Cast<ANF_MonsterAIController>(GetController());

    if (!MonsterAIController)
    {
		UE_LOG(LogTemp, Warning, TEXT("Attack failed: MonsterAIController is null"));
        return;
    }

    AActor* TargetActor = MonsterAIController->GetCurrentTargetActor();

    if (!IsValid(TargetActor) || TargetActor == this)
    {
		UE_LOG(LogTemp, Warning, TEXT("Attack failed: TargetActor is invalid"));
        return;
    }

    const float DistanceSquared = FVector::DistSquared(
        GetActorLocation(),
        TargetActor->GetActorLocation());

    if (DistanceSquared > FMath::Square(BasicAttackHitRange))
    {
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Attack failed: Target out of range. Distance: %.1f / Range: %.1f"),
			FMath::Sqrt(DistanceSquared),
			BasicAttackHitRange);
        return;
    }

    const FVector DirectionToTarget =
        (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();

    const float ForwardDot =
        FVector::DotProduct(GetActorForwardVector(), DirectionToTarget);
	
    if (ForwardDot < 0.3f)
    {
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Attack failed: Target is outside forward angle. Dot: %.2f"),
			ForwardDot);
        return;
    }
    

    IAbilitySystemInterface* TargetAbilitySystemInterface =
        Cast<IAbilitySystemInterface>(TargetActor);

    if (!TargetAbilitySystemInterface)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Monster attack failed: Target has no AbilitySystemComponent."));

        return;
    }

    UAbilitySystemComponent* TargetASC =
        TargetAbilitySystemInterface->GetAbilitySystemComponent();

    if (!TargetASC)
    {
		UE_LOG(LogTemp, Warning, TEXT("Attack failed: Target ASC is null"));
        return;
    }

    FGameplayEffectContextHandle EffectContext =
        MonsterASC->MakeEffectContext();

    EffectContext.AddSourceObject(this);

    FGameplayEffectSpecHandle EffectSpecHandle =
        MonsterASC->MakeOutgoingSpec(
            BasicAttackDamageEffect,
            1.0f,
            EffectContext);

    if (!EffectSpecHandle.IsValid())
    {
		UE_LOG(LogTemp, Warning, TEXT("Attack failed: Damage EffectSpec is invalid"));
        return;
    }

    EffectSpecHandle.Data->SetSetByCallerMagnitude(
    NFTag_BossCombat::Data_Damage,
FMath::Max(MonsterAttributeSet->GetAttackPower(), 0.0f));
	
	const float TargetHealthBefore =
	TargetASC->GetNumericAttribute(
		UNF_PlayerStatusAttribute::GetHealthAttribute());

    MonsterASC->ApplyGameplayEffectSpecToTarget(
        *EffectSpecHandle.Data.Get(),
        TargetASC);
	
	const float TargetHealthAfter =
	TargetASC->GetNumericAttribute(
		UNF_PlayerStatusAttribute::GetHealthAttribute());

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Player Health: %.1f -> %.1f"),
		TargetHealthBefore,
		TargetHealthAfter);
}

void ANF_Monster::HandleMonsterDamaged(float Damage, AActor* DamageSource)
{
	if (HasAuthority())
	{
		const APawn* SourcePawn = Cast<APawn>(DamageSource);
		if (IsValid(SourcePawn) && SourcePawn->IsPlayerControlled())
		{
			MulticastRevealStatusToAttacker(SourcePawn->GetPlayerState());
		}
	}
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Monster damage callback: Monster=%s / Source=%s / Controller=%s"),
		*GetNameSafe(this),
		*GetNameSafe(DamageSource),
		*GetNameSafe(GetController()));

	ANF_MonsterSpawner* MonsterSpawner = GuardSpawner.Get();
	if (!IsValid(MonsterSpawner))
	{
		MonsterSpawner = Cast<ANF_MonsterSpawner>(GetOwner());
	}

	if (!IsValid(MonsterSpawner))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Monster guard group skipped: Monster=%s has no assigned guard spawner. Owner=%s"),
			*GetNameSafe(this),
			*GetNameSafe(GetOwner()));
	}

	const bool bGroupAggroHandled = MonsterSpawner
		&& MonsterSpawner->TryAlertGuardGroup(DamageSource);

	if (!bGroupAggroHandled)
	{
		if (ANF_MonsterAIController* MonsterAIController =
			Cast<ANF_MonsterAIController>(GetController()))
		{
			MonsterAIController->ApplyDamageAggro(DamageSource);
		}
		else
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Monster damage aggro skipped: %s has no ANF_MonsterAIController."),
				*GetNameSafe(this));
		}
	}
	
	if (!MonsterASC) { return; }

	MonsterASC->AddLooseGameplayTag(
		NFTag_Monster::State_Monster_HitReact);

	const float MontageDuration = PlayAnimMontage(HitReactMontage);

	if (MontageDuration <= 0.0f)
	{
		ClearHitReactState();

		return;
	}

	GetWorldTimerManager().SetTimer(
		HitReactTimerHandle,
		this,
		&ThisClass::ClearHitReactState,
		MontageDuration,
		false);
}

void ANF_Monster::ClearHitReactState()
{
	if (!MonsterASC) { return; }

	MonsterASC->RemoveLooseGameplayTag(
		NFTag_Monster::State_Monster_HitReact);
}

void ANF_Monster::HandleMonsterOutOfHealth()
{
	UE_LOG(
	LogTemp,
	Warning,
	TEXT("HandleMonsterOutOfHealth executed: %s"),
	*GetName());
	
	if (bIsDead || !MonsterASC)	{ return; }

	bIsDead = true;
	if (IsValid(MonsterStatusWidgetComponent)) MonsterStatusWidgetComponent->SetVisibility(false);
	
	if (HasAuthority())
	{
		GrantExperienceToKiller();
		SpawnMonsterDrops();
	}
	
	OnMonsterDied.Broadcast(this);

	MonsterASC->RemoveLooseGameplayTag(
		NFTag_Monster::State_Monster_HitReact);

	MonsterASC->AddLooseGameplayTag(
		NFTag_Monster::State_Monster_Dead);

	MonsterASC->CancelAllAbilities();

	if (ANF_MonsterAIController* MonsterAIController =
		Cast<ANF_MonsterAIController>(GetController()))
	{
		MonsterAIController->ClearCurrentTargetActor(true);
	}

	if (AAIController* MonsterAIController =
		Cast<AAIController>(GetController()))
	{
		MonsterAIController->StopMovement();

		if (UBrainComponent* BrainComponent =
			MonsterAIController->GetBrainComponent())
		{
			BrainComponent->StopLogic(TEXT("MonsterDead"));
		}
	}

	GetCharacterMovement()->DisableMovement();

	GetCapsuleComponent()->SetCollisionEnabled(
		ECollisionEnabled::NoCollision);

	PlayAnimMontage(DeathMontage);

	SetLifeSpan(CorpseLifeSpan);
}

void ANF_Monster::HandleMoveSpeedChanged(
	const FOnAttributeChangeData& Data)
{
	SetMonsterMoveSpeed(Data.NewValue);
}

void ANF_Monster::SetMonsterMoveSpeed(float NewMoveSpeed)
{
	UCharacterMovementComponent* MonsterMovementComponent =
		GetCharacterMovement();

	if (!MonsterMovementComponent)
	{
		return;
	}

	MonsterMovementComponent->MaxWalkSpeed = NewMoveSpeed;
}

FVector ANF_Monster::GetHomeLocation() const
{
	return HomeLocation;
}

void ANF_Monster::ApplyGroupDamageAggro(AActor* TargetActor)
{
	ANF_MonsterAIController* MonsterAIController =
		Cast<ANF_MonsterAIController>(GetController());

	if (!MonsterAIController) { return; }

	MonsterAIController->ApplyDamageAggro(TargetActor);
}

void ANF_Monster::SetGuardFocusActor(AActor* InGuardFocusActor)
{
	GuardFocusActor = InGuardFocusActor;

	ANF_MonsterAIController* MonsterAIController =
		Cast<ANF_MonsterAIController>(GetController());

	if (!MonsterAIController) { return; }

	MonsterAIController->SetGuardFocusActor(GuardFocusActor.Get());
}

void ANF_Monster::SetGuardSpawner(ANF_MonsterSpawner* InGuardSpawner)
{
	GuardSpawner = InGuardSpawner;
}

void ANF_Monster::SpawnMonsterDrops()
{
	UWorld* World = GetWorld();
	if (!World)	{ return; }

	for (const FNF_MonsterDropEntry& DropEntry : DropEntries)
	{
		const float DropChance = FMath::Clamp(DropEntry.DropChance, 0.0f, 1.0f);
		if (!DropEntry.DropActorClass || FMath::FRand() > DropChance)
		{
			continue;
		}

		FVector DropLocation;
		if (!FindMonsterDropLocation(DropLocation))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("Monster drop skipped because no ground was found. Monster=%s"),
				*GetNameSafe(this));
			continue;
		}

		const int32 MinimumQuantity = FMath::Max(DropEntry.MinQuantity, 1);
		const int32 MaximumQuantity = FMath::Max(
			DropEntry.MaxQuantity,
			MinimumQuantity);
		const int32 Quantity = FMath::RandRange(
			MinimumQuantity,
			MaximumQuantity);

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.Instigator = this;
		SpawnParameters.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

		ANF_MonsterItemDrop* SpawnedDrop = World->SpawnActor<ANF_MonsterItemDrop>(
			DropEntry.DropActorClass,
			DropLocation,
			FRotator::ZeroRotator,
			SpawnParameters);

		if (IsValid(SpawnedDrop))
		{
			SpawnedDrop->InitializeDrop(Quantity);
		}
	}
}

bool ANF_Monster::FindMonsterDropLocation(FVector& OutDropLocation) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const float RandomAngle = FMath::FRandRange(0.0f, 2.0f * PI);
	const float RandomDistance = FMath::Sqrt(FMath::FRand()) * DropScatterRadius;
	const FVector CandidateLocation = GetActorLocation() + FVector(
		FMath::Cos(RandomAngle) * RandomDistance,
		FMath::Sin(RandomAngle) * RandomDistance,
		0.0f);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(MonsterItemDropGroundTrace),
		false,
		this);

	FHitResult GroundHit;
	const FVector TraceStart = CandidateLocation + FVector(0.0f, 0.0f, 500.0f);
	const FVector TraceEnd = CandidateLocation - FVector(0.0f, 0.0f, 2000.0f);

	if (!World->LineTraceSingleByObjectType(
		GroundHit,
		TraceStart,
		TraceEnd,
		ObjectQueryParams,
		QueryParams))
	{
		return false;
	}

	OutDropLocation = GroundHit.ImpactPoint + GroundHit.ImpactNormal * 5.0f;
	return true;
}

void ANF_Monster::ApplyLandingDamage(
	const FVector& LandingLocation,
	float DamageRadius,
	float DamageMultiplier)
{
	if (!HasAuthority()) { return; }

	if (!MonsterASC
		|| !MonsterAttributeSet
		|| !BasicAttackDamageEffect)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Monster leap damage failed: Damage setup is invalid."));

		return;
	}

	UWorld* World = GetWorld();

	if (!World)	{ return; }

	const float SafeRadius = FMath::Max(DamageRadius, 0.0f);

	if (SafeRadius <= 0.0f)	{ return; }

	FCollisionObjectQueryParams ObjectQueryParams;

	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(MonsterLeapDamage),
		false,
		this);

	TArray<FOverlapResult> OverlapResults;

	const bool bFoundOverlap = World->OverlapMultiByObjectType(
		OverlapResults,
		LandingLocation,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(SafeRadius),
		QueryParams);

	if (!bFoundOverlap)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("Monster leap damage: No targets in range."));

		return;
	}

	const float DamageAmount =
	FMath::Max(MonsterAttributeSet->GetAttackPower(), 0.0f)
	* FMath::Max(DamageMultiplier, 0.0f);

	TSet<UAbilitySystemComponent*> DamagedTargetASCs;

	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		APawn* TargetPawn =
			Cast<APawn>(OverlapResult.GetActor());

		// 다른 몬스터·일반 Pawn은 제외하고 실제 플레이어만 피해 처리
		if (!IsValid(TargetPawn)
			|| !TargetPawn->IsPlayerControlled()) {	continue; }

		IAbilitySystemInterface* TargetAbilitySystemInterface =
			Cast<IAbilitySystemInterface>(TargetPawn);

		if (!TargetAbilitySystemInterface)	{ continue;	}

		UAbilitySystemComponent* TargetASC =
			TargetAbilitySystemInterface->GetAbilitySystemComponent();

		// 하나의 Player ASC에는 도약 1회당 피해를 한 번만 줌
		if (!TargetASC
			|| DamagedTargetASCs.Contains(TargetASC)) {	continue; }

		FGameplayEffectContextHandle EffectContext =
			MonsterASC->MakeEffectContext();

		EffectContext.AddSourceObject(this);

		FGameplayEffectSpecHandle EffectSpecHandle =
			MonsterASC->MakeOutgoingSpec(
				BasicAttackDamageEffect,
				1.0f,
				EffectContext);

		if (!EffectSpecHandle.IsValid())
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Monster leap damage failed: EffectSpec is invalid."));

			continue;
		}

		EffectSpecHandle.Data->SetSetByCallerMagnitude(
	NFTag_BossCombat::Data_Damage,
	DamageAmount);

		MonsterASC->ApplyGameplayEffectSpecToTarget(
			*EffectSpecHandle.Data.Get(),
			TargetASC);

		DamagedTargetASCs.Add(TargetASC);

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Monster leap hit: %s / Damage: %.1f"),
			*GetNameSafe(TargetPawn),
			-DamageAmount);
	}
}

void ANF_Monster::GrantExperienceToKiller()
{
	if (!HasAuthority() || !IsValid(MonsterAttributeSet)) { return; }

	if (!ExperienceRewardEffect
		|| !ExperienceRewardMagnitudeTag.IsValid()
		|| !FMath::IsFinite(ExperienceReward)
		|| ExperienceReward <= 0.0f) {	return;	}

	ANF_PlayerState* KillerPlayerState = Cast<ANF_PlayerState>(
		MonsterAttributeSet->GetLethalDamageInstigatorPlayerState());

	if (!IsValid(KillerPlayerState)) { return; }

	UAbilitySystemComponent* KillerASC =
		KillerPlayerState->GetAbilitySystemComponent();

	if (!IsValid(KillerASC)
		|| !KillerASC->GetSet<UNF_PlayerLevelAttribute>()) { return; }

	FGameplayEffectContextHandle Context = KillerASC->MakeEffectContext();
	Context.AddSourceObject(this);

	FGameplayEffectSpecHandle Spec = KillerASC->MakeOutgoingSpec(
		ExperienceRewardEffect,
		1.0f,
		Context);

	if (!Spec.IsValid()) { return; }

	Spec.Data->SetSetByCallerMagnitude(
		ExperienceRewardMagnitudeTag,
		ExperienceReward);

	KillerASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}