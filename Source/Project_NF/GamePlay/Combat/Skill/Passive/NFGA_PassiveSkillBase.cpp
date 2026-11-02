#include "GamePlay/Combat/Skill/Passive/NFGA_PassiveSkillBase.h"

UNFGA_PassiveSkillBase::UNFGA_PassiveSkillBase()
{
	SkillType = ENF_SkillType::Passive;
	SkillActivationMethod = ENF_SkillActivationMethod::OnGranted;
}
