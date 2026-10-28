#include "NFTag_LifeState.h"

namespace NFTag_LifeState
{
	// @Owner: 지우 | 2026-10-16
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_Life_Downed,
		"State.Life.Downed",
		"플레이어 기절(Downed) 상태");
	
	// @Owner: 지우 | 2026-10-16
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_Life_Dead,
		"State.Life.Dead",
		"플레이어 사망 상태");
	
	// @Owner: 지우 | 2026-10-22
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Ability_Life_Downed,
		"Ability.Life.Downed",
		" 기절 상태 진입 시 실행하는 GA");
	
	// @Owner: 지우 | 2026-10-28
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Ability_Life_Revive,
		"Ability.Life.Revive",
		"기절한 팀원을 소생하는 GA 식별 및 활성화용 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Data_Life_ReviveHealth,
		"Data.Life.ReviveHealth",
		"소생 시 최대 체력을 기준으로 계산한 회복량을 GE에 전달");
}
