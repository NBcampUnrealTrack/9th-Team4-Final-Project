#include"NF_PlayerDataAsset.h"

#include "GAS/Abilities/NFGA_Downed.h"
#include "GAS/Abilities/NFGA_MeleeHitCheck.h"
#include "GAS/Abilities/NFGA_PlayerDeath.h"
#include "Player/Character/NFGA_MeleeAttack.h"


UNF_PlayerDataAsset::UNF_PlayerDataAsset()
{
	InitGA_Array.Add(UNFGA_PlayerDeath::StaticClass());
	InitGA_Array.Add(UNFGA_Downed::StaticClass());
	InitGA_Array.Add(UNFGA_MeleeAttack::StaticClass());
	InitGA_Array.Add(UNFGA_MeleeHitCheck::StaticClass());
}
