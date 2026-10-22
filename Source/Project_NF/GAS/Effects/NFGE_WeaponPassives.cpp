#include "GAS/Effects/NFGE_WeaponPassives.h"
#include "GAS/Attributes/NF_OffenseAttributeSet.h"
#include "GAS/Tags/NFTag_Skill.h"

UNFGE_AttackDamageBonus::UNFGE_AttackDamageBonus()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UNF_OffenseAttributeSet::GetAttackDamageBonusAttribute();
	Modifier.ModifierOp = EGameplayModOp::AddBase;
	FSetByCallerFloat Magnitude; Magnitude.DataTag = NFTag_Skill::Data_Weapon_AttackDamageBonus;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Magnitude);
	Modifiers.Add(Modifier);
}
UNFGE_MagicBullet::UNFGE_MagicBullet()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UNF_OffenseAttributeSet::GetLastRoundDamageMultiplierAttribute();
	Modifier.ModifierOp = EGameplayModOp::MultiplyCompound;
	FSetByCallerFloat Magnitude; Magnitude.DataTag = NFTag_Skill::Data_Weapon_LastRoundMultiplier;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Magnitude);
	Modifiers.Add(Modifier);
}
