#include "GamePlay/BuildSystem/Player/NF_BuildEquipmentAdapter.h"

#include "GamePlay/Combat/Armor/NF_ArmorBase.h"
#include "GamePlay/Combat/Armor/NF_ArmorDataAsset.h"
#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"
#include "GamePlay/Combat/Weapons/NF_WeaponDataAsset.h"
#include "GamePlay/BuildSystem/Core/NF_BuildWeaponDataReceiver.h"

bool FNF_BuildEquipmentAdapter::IsArmorDefinitionCompatible(const FNF_BuildItemData& Data)
{
	if (Data.ItemType != ENF_ItemType::Armor || !Data.ArmorClass ||
		Data.ArmorClass->HasAnyClassFlags(CLASS_Abstract))
	{
		return false;
	}
	return IsValid(Data.ArmorData) && Data.ArmorData->ItemType == ENF_ItemType::Armor &&
		Data.ArmorData->ArmorClass == Data.ArmorClass &&
		FNF_BuildItemSlots::GetDefaultSlotIndex(Data) != INDEX_NONE &&
		Data.ArmorData->ArmorSlot == Data.ArmorSlot;
}

bool FNF_BuildEquipmentAdapter::ApplyWeaponData(ANF_WeaponBase* Weapon, const FNF_BuildItemData& Data)
{
	if (!IsValid(Weapon) || !Weapon->HasAuthority() || !IsValid(Data.WeaponData)) return false;
	if (Weapon->GetWeaponData() == Data.WeaponData.Get()) return true;
	INF_BuildWeaponDataReceiver* Receiver = Cast<INF_BuildWeaponDataReceiver>(Weapon);
	return Receiver && Receiver->ApplyBuildWeaponData(Data.WeaponData.Get()) &&
		Weapon->GetWeaponData() == Data.WeaponData.Get();
}
