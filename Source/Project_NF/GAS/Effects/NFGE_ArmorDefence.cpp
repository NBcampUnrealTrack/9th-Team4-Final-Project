#include "GAS/Effects/NFGE_ArmorDefence.h"
#include "GAS/Tags/NFTag_Armor.h"
#include "Player/Character/NF_PlayerStatusAttribute.h"

UNFGE_ArmorDefence::UNFGE_ArmorDefence()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = UNF_PlayerStatusAttribute::GetDefenceAttribute();
	Modifier.ModifierOp = EGameplayModOp::AddBase;
	FSetByCallerFloat Magnitude;
	Magnitude.DataTag = NFTag_Armor::Data_Armor_BaseDefence;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Magnitude);
}
