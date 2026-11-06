
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
	// @Owner: 명현 | 2026-10-22
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player_Event_HitCheckEnd, "Player.Event.HitCheckEnd", "해당 Event가 Send 되면 HitCheck 종료")
	// @Owner: 명현 | 2026-10-22
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player_Event_Destroy, "Player.Event.Destroy", "해당 Event가 Send 되면 액터 삭제")
	// @Owner: 명현 | 2026-10-23
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player_Event_DamagedCheck, "Player.Event.DamagedCheck", "해당 Event가 Send 되면 현재 받을 데미지 전송")
	// @Owner: 명현 | 2026-10-23
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player_Event_HitReact, "Player.Event.HitReact", "해당 Event가 Send 되면 히트 리액션 발동")
	
	// @Owner: 명현 | 2026-10-21
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player_State_Death, "Player.State.Death", "죽음 상태를 의미하며, 다른 GA들의 사용을 막음")
	// @Owner: 명현 | 2026-10-21
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player_State_Attack, "Player.State.Attack", "플레이어가 공격중인 상태")
	
	// @Owner: 용준 | 2026-10-21
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player_State_Reload, "Player.State.Reload", "리로드 시 공격 및 줌 기능이 금지 되게 하기 위함")
	
	
	// @Owner: 명현 | 2026-10-29
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player_Data_ShieldRecoveryAmount, "Player.Data.ShieldRecoveryAmount", "플레이어의 쉴드 회복 데이터 전송용")
	
	// @Owner: 명현 | 2026-10-29
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player_Data_ShieldRecoveryBlockedDuration, 
		"Player.Data.ShieldRecoveryBlockedDuration", "플레이어의 쉴드 회복 불가 상태의 지속 시간 데이터 전송용")
	
	// @Owner: 명현 | 2026-10-30
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player_State_ShieldRecoveryBlocked, "Player.State.ShieldRecoveryBlocked", "플레이어가 쉴드 회복이 불가능한 상태")
	
	// @Owner: 명현 | 2026-10-30
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_InComingEXP, "Data.InComingEXP", "플레이어에게 경험치를 전달하는 태그")
	
	// @Owner: 명현 | 2026-11-05
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage, "Data.Damage", "플레이어에게 경험치를 전달하는 태그")
}