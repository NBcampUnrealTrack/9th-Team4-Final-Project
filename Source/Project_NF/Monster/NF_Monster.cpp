#include "Monster/NF_Monster.h"
#include "GameplayEffect.h"
#include "GAS/Foundation/NF_AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayAbilitySpec.h"
#include "GAS/Attributes/NF_MonsterAttributeSet.h"

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


