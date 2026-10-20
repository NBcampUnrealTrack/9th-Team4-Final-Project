#include "NF_BuildEquipmentAdapter.h"
#include "GamePlay/BuildSystem/Data/NF_BuildItemDefinitionDataAsset.h"
#include "GamePlay/Combat/Armor/NF_ArmorBase.h"
#include "GamePlay/Combat/Armor/NF_ArmorDataAsset.h"

// @Owner: 황준현 | 2026-10-20
// 두 시스템의 enum 숫자에 의존하지 않고 부위를 연결한다.
ENF_ArmorSlot FNF_BuildEquipmentAdapter::ResolveArmorSlot(
	ENF_BuildArmorPart Part)
{
	switch (Part)
	{
	case ENF_BuildArmorPart::Head:
		return ENF_ArmorSlot::HeadArmor;

	case ENF_BuildArmorPart::UpperBody:
		return ENF_ArmorSlot::BodyArmor;

	case ENF_BuildArmorPart::LowerBody:
		return ENF_ArmorSlot::LegArmor;

	default:
		return ENF_ArmorSlot::Unknown;
	}
}

bool FNF_BuildEquipmentAdapter::IsArmorDefinitionCompatible(
	const UNF_BuildItemDefinitionDataAsset* Definition)
{
	if (!IsValid(Definition) ||
		Definition->ItemType != ENF_BuildItemType::Armor ||
		!Definition->ArmorClass ||
		Definition->ArmorClass->HasAnyClassFlags(CLASS_Abstract))
	{
		return false;
	}

	const ANF_ArmorBase* ArmorDefaults =
		Definition->ArmorClass->GetDefaultObject<ANF_ArmorBase>();

	const ENF_ArmorSlot ExpectedSlot =
		ResolveArmorSlot(Definition->ArmorPart);

	return IsValid(ArmorDefaults) &&
		IsValid(ArmorDefaults->GetArmorData()) &&
		ExpectedSlot != ENF_ArmorSlot::Unknown &&
		ArmorDefaults->GetArmorSlot() == ExpectedSlot;
}