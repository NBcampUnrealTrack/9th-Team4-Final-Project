#include "GamePlay/BuildSystem/Player/NF_BuildEquipmentAdapter.h"

#include "GamePlay/Combat/Armor/NF_ArmorBase.h"
#include "GamePlay/Combat/Armor/NF_ArmorDataAsset.h"

bool FNF_BuildEquipmentAdapter::IsArmorDefinitionCompatible(const FNF_BuildItemData& Data)
{
	if (Data.ItemType != ENF_ItemType::Armor || !Data.ArmorClass ||
		Data.ArmorClass->HasAnyClassFlags(CLASS_Abstract))
	{
		return false;
	}
	const ANF_ArmorBase* ArmorDefaults = Data.ArmorClass->GetDefaultObject<ANF_ArmorBase>();
	return IsValid(ArmorDefaults) && IsValid(ArmorDefaults->GetArmorData()) &&
		FNF_BuildItemSlots::GetDefaultSlotIndex(Data) != INDEX_NONE &&
		ArmorDefaults->GetArmorSlot() == Data.ArmorSlot;
}
