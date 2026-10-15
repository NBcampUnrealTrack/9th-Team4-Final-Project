#include "GAS/Tags/NFTag_BossCombat.h"

namespace NFTag_BossCombat
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage, "Data.Damage", "GameplayEffect가 읽는 양수 피해 SetByCaller 값.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Stunned, "State.Stunned", "지속 시간 동안 플레이어 이동을 제한하는 상태.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Airborne, "State.Airborne", "보스 공격으로 공중에 뜬 상태.");
}
