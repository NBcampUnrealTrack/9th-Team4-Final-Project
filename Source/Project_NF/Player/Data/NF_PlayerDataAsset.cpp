#include"NF_PlayerDataAsset.h"

#include "GAS/Abilities/NFGA_Downed.h"
#include "GAS/Abilities/NFGA_HitReact.h"
#include "GAS/Abilities/NFGA_MeleeHitCheck.h"
#include "GAS/Abilities/NFGA_PlayerDeath.h"
#include "GAS/Abilities/NFGA_Revive.h"
#include "Player/Character/NFGA_MeleeAttack.h"


UNF_PlayerDataAsset::UNF_PlayerDataAsset()
{
	NF_GameplayAbility_Array.Add(UNFGA_PlayerDeath::StaticClass());
	NF_GameplayAbility_Array.Add(UNFGA_Downed::StaticClass());
	NF_GameplayAbility_Array.Add(UNFGA_MeleeAttack::StaticClass());
	NF_GameplayAbility_Array.Add(UNFGA_MeleeHitCheck::StaticClass());
	NF_GameplayAbility_Array.Add(UNFGA_Revive::StaticClass());
	NF_GameplayAbility_Array.Add(UNFGA_HitReact::StaticClass());
}
