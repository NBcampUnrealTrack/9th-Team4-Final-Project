
#include "NF_ConsumableSlotViewModel.h"

void UNF_ConsumableSlotViewModel::UpdateSlot(const FNF_ConsumableSlotViewData& NewData)
{
	const int32 NewQuantity = FMath::Max(0, NewData.Quantity);

	bool bNewCanUse = false;

	if (!NewData.ItemId.IsNone() && NewQuantity > 0)
	{
		bNewCanUse = NewData.bUsageAllowed;
	}

	UE_MVVM_SET_PROPERTY_VALUE(SelectedItemId, NewData.ItemId);
	UE_MVVM_SET_PROPERTY_VALUE(SelectedItemName, NewData.DisplayName);
	UE_MVVM_SET_PROPERTY_VALUE(SelectedItemIcon, NewData.Icon);
	UE_MVVM_SET_PROPERTY_VALUE(Quantity, NewQuantity);
	UE_MVVM_SET_PROPERTY_VALUE(ShortcutText, NewData.ShortcutText);
	UE_MVVM_SET_PROPERTY_VALUE(bCanUse, bNewCanUse);
}