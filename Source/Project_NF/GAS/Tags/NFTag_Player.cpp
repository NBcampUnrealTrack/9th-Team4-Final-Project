
#include "NFTag_Player.h"

namespace  NFTag_Player
{
	// @Owner: 명현 | 2026-10-13
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_Ability_Death, "Character.Ability.Death", "(현재 Owner가 사용하지않고 있음) 플레이어의 죽음 GA 실행 용도")
	
	// @Owner: 명현 | 2026-10-14
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player_Ability_Death, "Player.Ability.Death", "Character_Ability_Death -> Player_Ability_Death 로 변경 됨")
	// @Owner: 명현 | 2026-10-19
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player_State_Traversal, "Player.State.Traversal", "파쿠르 시 공격 및 줌 기능이 금지 되게 하기 위함")
	
	// @Owner: 명현 | 2026-10-21
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player_Event_HitCheck, "Player.Event.HitCheck", "해당 Event가 Send 되면 HitCheck가 발동 됨")
	// @Owner: 명현 | 2026-10-21
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player_Event_HitCheckEnd, "Player.Event.HitCheckEnd", "해당 Event가 Send 되면 HitCheck 종료")
	// @Owner: 명현 | 2026-10-21
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player_Event_Destroy, "Player.Event.Destroy", "해당 Event가 Send 되면 액터 삭제")
	
	// @Owner: 명현 | 2026-10-21
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player_State_Death, "Player.State.Death", "죽음 상태를 의미하며, 다른 GA들의 사용을 막음")
	// @Owner: 명현 | 2026-10-21
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player_State_Attack, "Player.State.Attack", "플레이어가 공격중인 상태")
	
	// @Owner: 용준 | 2026-10-21
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player_State_Reload, "Player.State.Reload", "리로드 시 공격 및 줌 기능이 금지 되게 하기 위함")
}