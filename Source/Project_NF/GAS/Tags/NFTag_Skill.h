#pragma once

#include "NativeGameplayTags.h"

namespace NFTag_Skill
{
	// @Owner: 이정준 (구현: Codex) | 2026-10-22 | 패시브 발동 연출 식별자 (조건 태그 아님)
	PROJECT_NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Skill_Thorns_Activate);
	PROJECT_NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Skill_Berserker_Activate);
	PROJECT_NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Skill_DoubleJump_Activate);
	// @Owner: 이정준 (구현: Codex) | 2026-10-22 | 가시갑옷 반사량 및 재반사 방지
	PROJECT_NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage_Reflected);
	PROJECT_NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Reflected);
	// @Owner: 이정준 (구현: Codex) | 2026-10-21 | 무기 공격 패시브 GE의 SetByCaller 값
	PROJECT_NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Weapon_AttackDamageBonus);
	PROJECT_NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Weapon_LastRoundMultiplier);
	// @Owner: 이정준 (구현: Codex) | 2026-10-20
	PROJECT_NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Debuff_EMP);
	// @Owner: 이정준 (구현: Codex) | 2026-10-20 | EMP 피격/지속 연출
	PROJECT_NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Debuff_EMP);
	// @owner : 이정준 | 2026-10-15
	PROJECT_NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_DoubleJumpEnabled);
}
