#include "GAS/Tags/NFTag_Skill.h"

namespace NFTag_Skill
{
	// @owner : 이정준 | 2026-10-15
	PROJECT_NF_API UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_DoubleJumpEnabled,
		"State.DoubleJumpEnabled",
		"더블 점프 패시브 장착으로 추가 점프를 허용하는 상태. 캐릭터에서 태그 유무에 따라 최대 점프 횟수를 계산한다.");
}
