#include "NFTag_Build.h"

namespace NFTag_Build
{
	// @Owner: 준현 | 2026-10-11
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_Build_Locked, "State.Build.Locked", "사망, 기절, 매치 단계 등에서 빌드 교체를 막기 위한 상태")

	// @Owner: 민웅 | 2026-10-16
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_Build_Ready, "State.Build.Ready", "서버에 출격 가능한 유효한 빌드가 확정 적용된 상태")
} // namespace NFTag_Build
