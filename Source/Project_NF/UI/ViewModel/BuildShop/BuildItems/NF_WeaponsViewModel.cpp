#include "NF_WeaponsViewModel.h"

#include "Data/Enum/NF_ItemTypes.h"
#include "GamePlay/BuildSystem/Core/NF_BuildItemSource.h"
#include "GamePlay/BuildSystem/Data/NF_BuildItemDefinitionDataAsset.h"
#include "GamePlay/Combat/Weapons/NF_WeaponDataAsset.h"

void UNF_WeaponsViewModel::Configure(
	const UNF_BuildItemDefinitionDataAsset* Item, const FNF_BuildItemData& ItemData,
	const UNF_WeaponDataAsset* WeaponData,
	UTexture2D* InIcon,
	const FText& InDescription)
{
	if (!IsValid(Item)) return;
	if (ItemData.ItemType != ENF_ItemType::Weapon) return;
	if (!IsValid(WeaponData))return;

	UE_MVVM_SET_PROPERTY_VALUE(ItemId, Item->ItemId);
	UE_MVVM_SET_PROPERTY_VALUE(DisplayName, Item->DisplayName);
	UE_MVVM_SET_PROPERTY_VALUE(Cost, ItemData.Cost);
	UE_MVVM_SET_PROPERTY_VALUE(Icon, InIcon);
	UE_MVVM_SET_PROPERTY_VALUE(Description, InDescription);

	UE_MVVM_SET_PROPERTY_VALUE(Damage, WeaponData->Damage);
	UE_MVVM_SET_PROPERTY_VALUE(RoundsPerMinute,WeaponData->RoundsPerMinute);
	UE_MVVM_SET_PROPERTY_VALUE(MagazineSize,WeaponData->MagazineSize);
	
	UE_MVVM_SET_PROPERTY_VALUE(FireModeText,GetFireModeText(WeaponData->FireMode));
}

FText UNF_WeaponsViewModel::GetFireModeText(ENF_FireMode FireMode) const
{

	if (FireMode == ENF_FireMode::Automatic)
	{
		 return NSLOCTEXT("NFBuildUI", "Automatic", "자동");
	}
	else
	{
		return NSLOCTEXT("NFBuildUI", "SemiAutomatic", "반자동");
	}
}
