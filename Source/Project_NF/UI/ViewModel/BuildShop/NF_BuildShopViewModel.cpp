// NF_BuildShopViewModel.cpp

#include "NF_BuildShopViewModel.h"

void UNF_BuildShopViewModel::UpdateShop(const FNF_BuildShopViewData& NewData)
{
	SkillSlots = NewData.SkillSlots;
	WeaponSlots = NewData.WeaponSlots;
	EquipmentSlots = NewData.EquipmentSlots;
	CatalogItems = NewData.CatalogItems;

	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SkillSlots);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(WeaponSlots);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(EquipmentSlots);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CatalogItems);

	UE_MVVM_SET_PROPERTY_VALUE(CurrentCost,NewData.DraftValidation.TotalCost);

	UE_MVVM_SET_PROPERTY_VALUE(MaxCost,NewData.DraftValidation.CostLimit);

	UE_MVVM_SET_PROPERTY_VALUE(ValidationMessage,NewData.DraftValidation.Message);

	bool bNewCanApplyBuild = false;

	if (NewData.bIsShopOpen &&
		!NewData.bIsAwaitingBuildResult &&
		!NewData.bIsChoosingActiveSkillSlot &&
		NewData.DraftValidation.Result == ENF_BuildResult::Success)
	{
		bNewCanApplyBuild = true;
	}

	UE_MVVM_SET_PROPERTY_VALUE(bCanApplyBuild,bNewCanApplyBuild);
}