#include "GAS/Tags/NFGameplayCueTags.h"

namespace NFGameplayCueTags
{
	//2026-10-14 강명우
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Boss_Telegraph,
		"GameplayCue.Boss.Telegraph", "서버가 지정한 보스 공격 바닥 경고 표시.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Boss_ProjectileImpact,
		"GameplayCue.Boss.ProjectileImpact", "보스 투사체가 목표 지점에 도착했을 때 재생하는 충돌 연출.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Boss_ThreatVolley_Impact,
		"GameplayCue.Boss.ThreatVolley.Impact", "무작위 대상 연속 발사 패턴 투사체의 착탄 연출.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Boss_TargetWarning,
		"GameplayCue.Boss.TargetWarning", "보스가 선택한 플레이어에게 표시하는 머리 위 경고.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Boss_Submerge,
		"GameplayCue.Boss.Submerge", "보스가 땅속으로 들어갈 때 재생하는 연출.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Boss_Emerge,
		"GameplayCue.Boss.Emerge", "보스가 땅에서 나올 때 재생하는 연출.");
}
