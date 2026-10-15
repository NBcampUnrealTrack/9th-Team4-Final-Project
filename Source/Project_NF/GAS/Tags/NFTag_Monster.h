#pragma once

#include "NativeGameplayTags.h"

namespace NFTag_Monster
{
	//@경환 | 2026-10-12
	PROJECT_NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Monster_Dead);
	PROJECT_NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Monster_Attacking);
	PROJECT_NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Monster_HitReact);
	PROJECT_NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Monster_Stunned);
	
	PROJECT_NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Monster_BasicAttack);
	
	// @Owner: 경환 | 2026-10-14
	PROJECT_NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Monster_Damage);
}