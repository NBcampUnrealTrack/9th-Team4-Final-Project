#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

namespace NFTag_Player
{
	// @Owner: 명현 | 2026-10-13 (삭제 예정)
	PROJECT_NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Ability_Death);

	// @Owner: 명현 | 2026-10-14
	PROJECT_NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_Ability_Death);
	// @Owner: 명현 | 2026-10-19
	PROJECT_NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_State_Traversal);
	
	// @Owner: 명현 | 2026-10-21
	PROJECT_NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_Event_HitCheck);
	// @Owner: 명현 | 2026-10-22
	PROJECT_NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_Event_HitCheckEnd);
	// @Owner: 명현 | 2026-10-22
	PROJECT_NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_Event_Destroy);
	// @Owner: 명현 | 2026-10-23
	PROJECT_NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_Event_DamagedCheck);
	
	// @Owner: 명현 | 2026-10-21
	PROJECT_NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_State_Death);
	
	// @Owner: 명현 | 2026-10-21
	PROJECT_NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_State_Attack);
	
	// @Owner: 용준 | 2026-10-21
    PROJECT_NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_State_Reload);
	
	// @Owner: 명현 | 2026-10-30
	PROJECT_NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_Data_ShieldRecoveryAmount);
	
	// @Owner: 명현 | 2026-10-30
	PROJECT_NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_Data_ShieldRecoveryBlockedDuration);
	
	// @Owner: 명현 | 2026-10-30
	PROJECT_NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_State_ShieldRecoveryBlocked);
};
