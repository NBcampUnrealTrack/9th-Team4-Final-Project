#include "GAS/Effects/NFGE_HealthPotion.h"

#include "GAS/Tags/NFTag_Consumable.h"
#include "Player/Character/NF_PlayerStatusAttribute.h"

UNFGE_HealthPotion::UNFGE_HealthPotion()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UNF_PlayerStatusAttribute::GetHealthAttribute();
	Modifier.ModifierOp = EGameplayModOp::AddBase;
	FSetByCallerFloat Amount;
	Amount.DataTag = NFTag_Consumable::Data_Consumable_HealthRestore;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Amount);
	Modifiers.Add(Modifier);
}
