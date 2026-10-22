#include"NF_PlayerDataAsset.h"

#include "GAS/Abilities/NFGA_Downed.h"
#include "GAS/Abilities/NFGA_MeleeHitCheck.h"
#include "GAS/Abilities/NFGA_PlayerDeath.h"
#include "Player/Character/NFGA_MeleeAttack.h"


UNF_PlayerDataAsset::UNF_PlayerDataAsset()
{
	GameplayAbilityArray.Add(UNFGA_PlayerDeath::StaticClass());
	GameplayAbilityArray.Add(UNFGA_Downed::StaticClass());
	GameplayAbilityArray.Add(UNFGA_MeleeAttack::StaticClass());
	GameplayAbilityArray.Add(UNFGA_MeleeHitCheck::StaticClass());
}
