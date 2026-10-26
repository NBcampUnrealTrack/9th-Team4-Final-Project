#include "GamePlay/Combat/Skill/Data/NF_SkillDataAsset.h"

#include "GamePlay/Combat/Skill/NFGA_SkillBase.h"

bool UNF_SkillDataAsset::GetBuildItemData_Implementation(FNF_BuildItemData& OutData) const
{
	OutData = FNF_BuildItemData();
	if (!AbilityClass || AbilityClass->HasAnyClassFlags(CLASS_Abstract) || AbilityLevel < 1) { return false; }
	const UNFGA_SkillBase* Defaults = AbilityClass->GetDefaultObject<UNFGA_SkillBase>();
	if (!IsValid(Defaults) || Defaults->GetBuildCost() < 0) { return false; }

	FNF_BuildItemData Data;
	switch (Defaults->GetSkillType())
	{
	case ENF_SkillType::Active: Data.ItemType = ENF_ItemType::ActiveSkill; break;
	case ENF_SkillType::Passive: Data.ItemType = ENF_ItemType::PassiveSkill; break;
	default: return false;
	}
	Data.Cost = Defaults->GetBuildCost();
	Data.AbilityClass = AbilityClass;
	Data.AbilityLevel = AbilityLevel;
	OutData = MoveTemp(Data);
	return true;
}
