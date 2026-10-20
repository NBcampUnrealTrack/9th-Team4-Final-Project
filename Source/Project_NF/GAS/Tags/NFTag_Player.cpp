
#include "NFTag_Player.h"

namespace  NFTag_Player
{
	// @Owner: 명현 | 2026-10-13
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_Ability_Death, "Character.Ability.Death", "(현재 Owner가 사용하지않고 있음) 플레이어의 죽음 GA 실행 용도")
	
	// @Owner: 명현 | 2026-10-14
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player_Ability_Death, "Player.Ability.Death", "Character_Ability_Death -> Player_Ability_Death 로 변경 됨")
	// @Owner: 명현 | 2026-10-19
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player_State_Traversal, "Player.State.Traversal", "파쿠르 시 공격 및 줌 기능이 금지 되게 하기 위함")
}