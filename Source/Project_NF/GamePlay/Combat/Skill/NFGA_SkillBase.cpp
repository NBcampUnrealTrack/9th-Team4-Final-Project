#include "GamePlay/Combat/Skill/NFGA_SkillBase.h"

// @Owner: 스킬 담당 | 2026-10-12
UNFGA_SkillBase::UNFGA_SkillBase()
{
	// 각 ASC의 스킬 인스턴스에서 실행 상태를 보관한다.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 초기 공통 베이스는 서버에서 실행한다. 예측이 필요한 스킬은 자식에서 정책을 설정한다.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}
