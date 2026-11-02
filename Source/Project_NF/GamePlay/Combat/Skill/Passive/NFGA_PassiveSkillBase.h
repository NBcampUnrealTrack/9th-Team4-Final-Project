#pragma once

#include "GamePlay/Combat/Skill/NFGA_SkillBase.h"
#include "NFGA_PassiveSkillBase.generated.h"

UCLASS(Blueprintable, HideCategories = ("NF|Skill|Input", "NF|Skill|Preview", "NF|Skill|Targeting", "NF|Skill|Execution", "NF|Skill|Animation", "NF|Skill|EMP"))
class PROJECT_NF_API UNFGA_PassiveSkillBase : public UNFGA_SkillBase
{
	GENERATED_BODY()
public:
	UNFGA_PassiveSkillBase();
};
