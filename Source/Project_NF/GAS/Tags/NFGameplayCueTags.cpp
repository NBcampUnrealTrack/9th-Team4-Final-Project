#include "GAS/Tags/NFGameplayCueTags.h"

namespace NFGameplayCueTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Boss_Telegraph,
		"GameplayCue.Boss.Telegraph", "서버가 지정한 보스 공격 바닥 경고 표시.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Boss_ProjectileImpact,
		"GameplayCue.Boss.ProjectileImpact", "보스 투사체가 목표 지점에 도착했을 때 재생하는 충돌 연출.");
}
