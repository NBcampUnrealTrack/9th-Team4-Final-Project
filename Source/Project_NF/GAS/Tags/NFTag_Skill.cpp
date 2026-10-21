#include "GAS/Tags/NFTag_Skill.h"

namespace NFTag_Skill
{
	// @Owner: 이정준 (구현: Codex) | 2026-10-22
	PROJECT_NF_API UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Skill_Thorns_Activate, "GameplayCue.Skill.Thorns.Activate", "가시갑옷 반사 성공 순간 연출. RawMagnitude는 반사 GE에 전달한 피해량.");
	PROJECT_NF_API UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Skill_Berserker_Activate, "GameplayCue.Skill.Berserker.Activate", "버서커 새 중첩 연출. RawMagnitude는 현재 중첩 수.");
	PROJECT_NF_API UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Skill_DoubleJump_Activate, "GameplayCue.Skill.DoubleJump.Activate", "실제 추가 점프 성공 시 호출할 연출. 장착 시점에는 호출하지 않는다.");
	// @Owner: 이정준 (구현: Codex) | 2026-10-22
	PROJECT_NF_API UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage_Reflected, "Data.Damage.Reflected",
		"실제 체력 손실에 가시갑옷 반사율을 곱해 계산한 반사 피해 GE SetByCaller 값.");
	PROJECT_NF_API UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Reflected, "Damage.Reflected",
		"반사 피해 Spec의 Asset Tag. 가시갑옷끼리 재반사되는 것을 방지한다.");
	// @Owner: 이정준 (구현: Codex) | 2026-10-21
	PROJECT_NF_API UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Weapon_AttackDamageBonus, "Data.Weapon.AttackDamageBonus",
		"무기 공격력 증가 패시브 GE에 전달하는 고정 추가 피해량. 무기 기본 피해에 합산한다.");
	PROJECT_NF_API UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Weapon_LastRoundMultiplier, "Data.Weapon.LastRoundMultiplier",
		"마탄 패시브 GE에 전달하는 마지막 탄환 피해 배율. 공격력 보너스 합산 이후 곱한다.");
	// @Owner: 이정준 (구현: Codex) | 2026-10-20
	PROJECT_NF_API UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Debuff_EMP, "GameplayCue.Debuff.EMP",
		"EMP 피격 및 지속 Gameplay Cue 연출. EMP GE와 GameplayCueNotify에서 동일한 태그를 사용한다.");
	// @Owner: 이정준 (구현: Codex) | 2026-10-20
	PROJECT_NF_API UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Debuff_EMP, "State.Debuff.EMP",
		"액티브 스킬의 새 발동을 봉인하고 유지 중인 스킬을 취소한다. 기본 이동/무기/패시브 및 이미 독립 생성된 투척물·설치물은 제외한다.");
	// @owner : 이정준 | 2026-10-15
	PROJECT_NF_API UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_DoubleJumpEnabled,
		"State.DoubleJumpEnabled",
		"더블 점프 패시브 장착으로 추가 점프를 허용하는 상태. 캐릭터에서 태그 유무에 따라 최대 점프 횟수를 계산한다.");
}
