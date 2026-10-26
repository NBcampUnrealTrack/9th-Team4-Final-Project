#include "GamePlay/Combat/Skill/Passive/Attack/NFGA_MagicBullet.h"

#include "GAS/Effects/NFGE_WeaponPassives.h"
#include "GAS/Tags/NFTag_Skill.h"

UNFGA_MagicBullet::UNFGA_MagicBullet()
{
	BuildCost = 3;
	StatEffectClass = UNFGE_MagicBullet::StaticClass();
	MagnitudeTag = NFTag_Skill::Data_Weapon_LastRoundMultiplier;
}
