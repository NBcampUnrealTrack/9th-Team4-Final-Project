#pragma once

#include "GamePlay/Combat/Skill/NFGA_SkillBase.h"
#include "NFGA_ActiveSkillBase.generated.h"

// 호환 전환 단계: 직렬화된 속성과 외부 입력 API는 공통 베이스에 유지한다.
UCLASS(Blueprintable, HideCategories = ("NF|Skill|Passive"))
class PROJECT_NF_API UNFGA_ActiveSkillBase : public UNFGA_SkillBase
{
	GENERATED_BODY()
public:
	UNFGA_ActiveSkillBase();
};
