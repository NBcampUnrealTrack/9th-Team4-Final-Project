#include"NF_PlayerDataAsset.h"

#include "GAS/Abilities/NFGA_MeleeHitCheck.h"
#include "Player/Character/NFGA_Death.h"
#include "Player/Character/NFGA_MeleeAttack.h"


UNF_PlayerDataAsset::UNF_PlayerDataAsset()
{
	GameplayAbilityArray.Add(UNFGA_Death::StaticClass());
	GameplayAbilityArray.Add(UNFGA_MeleeAttack::StaticClass());
	GameplayAbilityArray.Add(UNFGA_MeleeHitCheck::StaticClass());
}
