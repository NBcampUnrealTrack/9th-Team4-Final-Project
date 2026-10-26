#include "NF_ArmorsViewModel.h"

#include "Data/Enum/NF_ItemTypes.h"
#include "GamePlay/BuildSystem/Core/NF_BuildItemSource.h"
#include "GamePlay/BuildSystem/Data/NF_BuildItemDefinitionDataAsset.h"

void UNF_ArmorsViewModel::Configure(
	const UNF_BuildItemDefinitionDataAsset* Item, const FNF_BuildItemData& ItemData,
	UTexture2D* InIcon,
	const FText& InDescription,
	const FText& InEquipSlotText,
	float InDefenceBonus)
{
	if (!IsValid(Item)) return;
	if (ItemData.ItemType != ENF_ItemType::Armor) return;

	UE_MVVM_SET_PROPERTY_VALUE(ItemId, Item->ItemId);
	UE_MVVM_SET_PROPERTY_VALUE(DisplayName, Item->DisplayName);
	UE_MVVM_SET_PROPERTY_VALUE(Cost, ItemData.Cost);
	UE_MVVM_SET_PROPERTY_VALUE(Icon, InIcon);
	UE_MVVM_SET_PROPERTY_VALUE(Description, InDescription);

	UE_MVVM_SET_PROPERTY_VALUE(EquipSlotText,InEquipSlotText);

	UE_MVVM_SET_PROPERTY_VALUE(DefenceBonus,FMath::Max(0.0f, InDefenceBonus));
}
