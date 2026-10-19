#include "GAS/Tags/NFTag_Monster.h"

namespace NFTag_Monster
{
	//@경환 | 2026-10-12
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_Monster_Dead,
		"State.Monster.Dead",
		"몬스터가 사망하여 AI와 전투 행동을 수행할 수 없는 상태입니다.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_Monster_Attacking,
		"State.Monster.Attacking",
		"몬스터가 공격 Ability를 실행 중인 상태입니다.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_Monster_HitReact,
		"State.Monster.HitReact",
		"몬스터가 피격 반응을 수행 중인 상태입니다.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_Monster_Stunned,
		"State.Monster.Stunned",
		"몬스터가 기절하여 행동할 수 없는 상태입니다.");
	
	//@경환 | 2026-10-13
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Ability_Monster_BasicAttack,
		"Ability.Monster.BasicAttack",
		"일반 몬스터의 기본 공격 Ability");
	
	//@경환 | 2026-10-14
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
	Data_Monster_Damage,
	"Data.Monster.Damage",
	"몬스터가 플레이어에게 전달하는 피해량");
	
	// @Owner: 경환 | 2026-10-15
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Ability_Monster_RangedAttack,
		"Ability.Monster.RangedAttack",
		"원거리 몬스터 투사체 공격");
	
	// @Owner: 경환 | 2026-10-19
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Ability_Monster_LeapAttack,
		"Ability.Monster.LeapAttack",
		"몬스터가 목표 지점으로 도약하여 착지 피해를 주는 Ability");
}