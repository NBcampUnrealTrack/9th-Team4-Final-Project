#include "GamePlay/Combat/Weapons/NF_WeaponDataAsset.h"

#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"

bool UNF_WeaponDataAsset::GetBuildItemData_Implementation(FNF_BuildItemData& OutData) const
{
	OutData = FNF_BuildItemData();
	if (ItemType != ENF_ItemType::Weapon || Cost < 0 || !WeaponClass ||
		WeaponClass->HasAnyClassFlags(CLASS_Abstract))
	{
		return false;
	}

	OutData.ItemType = ItemType;
	OutData.Cost = Cost;
	OutData.WeaponClass = WeaponClass;
	return true;
}

