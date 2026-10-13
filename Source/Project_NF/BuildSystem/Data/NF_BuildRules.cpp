#include "BuildSystem/Data/NF_BuildRules.h"

int32 UNF_BuildRules::GetSlotCount(ENF_BuildItemType ItemType) const
{
	switch (ItemType)
	{
	case ENF_BuildItemType::Weapon:
		return WeaponSlots;
	case ENF_BuildItemType::Armor:
		return ArmorSlots;
	case ENF_BuildItemType::ActiveSkill:
		return ActiveSkillSlots;
	case ENF_BuildItemType::PassiveSkill:
		return PassiveSkillSlots;
	default:
		return 0;
	}
}

bool UNF_BuildRules::TryGetCostLimit(int32 PlayerLevel, int32& OutCostLimit) const
{
	OutCostLimit = 0;
	if (PlayerLevel < 1 || !CostLimitsByLevel.IsValidIndex(PlayerLevel - 1))
	{
		return false;
	}

	OutCostLimit = CostLimitsByLevel[PlayerLevel - 1];
	return OutCostLimit >= 0;
}
