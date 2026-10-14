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
