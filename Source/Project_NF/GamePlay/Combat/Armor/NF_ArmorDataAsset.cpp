#include "GamePlay/Combat/Armor/NF_ArmorDataAsset.h"

#include "GamePlay/Combat/Armor/NF_ArmorBase.h"

bool UNF_ArmorDataAsset::GetBuildItemData_Implementation(FNF_BuildItemData& OutData) const
{
	OutData = FNF_BuildItemData();
	if (ItemType != ENF_ItemType::Armor || Cost < 0 || !ArmorClass ||
		ArmorClass->HasAnyClassFlags(CLASS_Abstract) ||
		(ArmorSlot != ENF_ArmorSlot::HeadArmor && ArmorSlot != ENF_ArmorSlot::BodyArmor &&
		 ArmorSlot != ENF_ArmorSlot::LegArmor))
	{
		return false;
	}

	OutData.ItemType = ItemType;
	OutData.Cost = Cost;
	OutData.ArmorSlot = ArmorSlot;
	OutData.ArmorClass = ArmorClass;
	return true;
}
