#include "Monster/NF_Monster.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GAS/Attributes/NF_MonsterAttributeSet.h"

ANF_Monster::ANF_Monster()
{
	bReplicates = true;
 	
	AbilitySystemComponent =
		CreateDefaultSubobject<UAbilitySystemComponent>(
			TEXT("AbilitySystemComponent"));
	
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(
		EGameplayEffectReplicationMode::Minimal);

	MonsterAttributeSet =
		CreateDefaultSubobject<UNF_MonsterAttributeSet>(
			TEXT("MonsterAttributeSet"));	
}

UAbilitySystemComponent* ANF_Monster::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ANF_Monster::BeginPlay()
{
	Super::BeginPlay();
	
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	
	if (HasAuthority() && DefaultAttributesEffect)
	{
		FGameplayEffectContextHandle EffectContext =
			AbilitySystemComponent->MakeEffectContext();

		EffectContext.AddSourceObject(this);

		const FGameplayEffectSpecHandle EffectSpecHandle =
			AbilitySystemComponent->MakeOutgoingSpec(
				DefaultAttributesEffect,
				1.0f,
				EffectContext);

		if (EffectSpecHandle.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(
				*EffectSpecHandle.Data.Get());
		}
	}
}


