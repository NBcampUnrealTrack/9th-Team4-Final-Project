#include "GamePlay/BuildSystem/Core/NF_BuildTypes.h"

bool INF_BuildItemSource::GetBuildItemData_Implementation(FNF_BuildItemData& OutData) const
{
	OutData = FNF_BuildItemData();
	return false;
}

int32 FNF_BuildItemSlots::GetDefaultSlotIndex(const FNF_BuildItemData& Data)
{
	if (Data.ItemType != ENF_ItemType::Armor)
	{
		return 0;
	}
	switch (Data.ArmorSlot)
	{
	case ENF_ArmorSlot::HeadArmor: return 0;
	case ENF_ArmorSlot::BodyArmor: return 1;
	case ENF_ArmorSlot::LegArmor: return 2;
	default: return INDEX_NONE;
	}
}

bool FNF_BuildItemSlots::CanUseSlot(const FNF_BuildItemData& Data, int32 SlotIndex)
{
	return Data.ItemType == ENF_ItemType::ActiveSkill
		       ? SlotIndex >= 0 && SlotIndex < 2
		       : SlotIndex >= 0 && SlotIndex == GetDefaultSlotIndex(Data);
}
