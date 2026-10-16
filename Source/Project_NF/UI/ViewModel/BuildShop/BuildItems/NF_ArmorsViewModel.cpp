#include "NF_ArmorsViewModel.h"

#include "BuildSystem/Data/NF_BuildItemDefinition.h"

void UNF_ArmorsViewModel::Configure(
	const UNF_BuildItemDefinition* Item,
	UTexture2D* InIcon,
	const FText& InDescription,
	const FText& InEquipSlotText,
	float InDefenceBonus)
{
	if (!IsValid(Item)) return;
	if (Item->ItemType != ENF_BuildItemType::Armor) return;

	UE_MVVM_SET_PROPERTY_VALUE(ItemId, Item->ItemId);
	UE_MVVM_SET_PROPERTY_VALUE(DisplayName, Item->DisplayName);
	UE_MVVM_SET_PROPERTY_VALUE(Cost, Item->Cost);
	UE_MVVM_SET_PROPERTY_VALUE(Icon, InIcon);
	UE_MVVM_SET_PROPERTY_VALUE(Description, InDescription);

	UE_MVVM_SET_PROPERTY_VALUE(EquipSlotText,InEquipSlotText);

	UE_MVVM_SET_PROPERTY_VALUE(DefenceBonus,FMath::Max(0.0f, InDefenceBonus));
}