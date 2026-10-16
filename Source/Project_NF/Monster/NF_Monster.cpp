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

#include "Player/Character/NF_PlayerStatusAttribute.h"

ANF_Monster::ANF_Monster()
{
	bReplicates = true;

	MonsterASC =
	CreateDefaultSubobject<UNF_AbilitySystemComponent>(
		TEXT("MonsterASC"));

	MonsterASC->SetIsReplicated(true);
	MonsterASC->SetReplicationMode(
		EGameplayEffectReplicationMode::Minimal);

	MonsterAttributeSet =
		CreateDefaultSubobject<UNF_MonsterAttributeSet>(
			TEXT("MonsterAttributeSet"));
}

UAbilitySystemComponent* ANF_Monster::GetAbilitySystemComponent() const
{
	return MonsterASC.Get();
}

void ANF_Monster::BeginPlay()
{
	Super::BeginPlay();

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
	
	HomeLocation = GetActorLocation();
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
        NFTag_Monster::Data_Monster_Damage,
        -MonsterAttributeSet->GetAttackPower());
	
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
	if (ANF_MonsterAIController* MonsterAIController =
		Cast<ANF_MonsterAIController>(GetController()))
	{
		MonsterAIController->SetCurrentTargetActor(DamageSource);
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
	
	OnMonsterDied.Broadcast(this);

	MonsterASC->RemoveLooseGameplayTag(
		NFTag_Monster::State_Monster_HitReact);

	MonsterASC->AddLooseGameplayTag(
		NFTag_Monster::State_Monster_Dead);

	MonsterASC->CancelAllAbilities();

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