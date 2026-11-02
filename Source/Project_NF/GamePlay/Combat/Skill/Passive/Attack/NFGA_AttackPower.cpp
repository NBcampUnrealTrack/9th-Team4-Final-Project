#include "GamePlay/Combat/Skill/Passive/Attack/NFGA_AttackPower.h"

#include "GAS/Effects/NFGE_WeaponPassives.h"
#include "GAS/Tags/NFTag_Skill.h"

UNFGA_AttackPower::UNFGA_AttackPower()
{
	GetMutableSkillDefaults().BuildCost = 1;
	StatEffectClass = UNFGE_AttackDamageBonus::StaticClass();
	MagnitudeTag = NFTag_Skill::Data_Weapon_AttackDamageBonus;
}
