#include "NF_KeyHolderAttributeSet.h"
#include "Net/UnrealNetwork.h"

UNF_KeyHolderAttributeSet::UNF_KeyHolderAttributeSet()
{
	// 열쇠가 없으면 감소 효과x
	InitCooldownReductionRate(0.0f);
	InitDamageReductionRate(0.0f);
}

void UNF_KeyHolderAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(
		UNF_KeyHolderAttributeSet,
		CooldownReductionRate,
		COND_None,
		REPNOTIFY_Always);
	
	DOREPLIFETIME_CONDITION_NOTIFY(
		UNF_KeyHolderAttributeSet,
		DamageReductionRate,
		COND_None,
		REPNOTIFY_Always);
}

void UNF_KeyHolderAttributeSet::OnRep_CooldownReductionRate(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(
		UNF_KeyHolderAttributeSet,
		CooldownReductionRate,
		OldValue);
}

void UNF_KeyHolderAttributeSet::OnRep_DamageReductionRate(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(
		UNF_KeyHolderAttributeSet,
		DamageReductionRate,
		OldValue);
}
