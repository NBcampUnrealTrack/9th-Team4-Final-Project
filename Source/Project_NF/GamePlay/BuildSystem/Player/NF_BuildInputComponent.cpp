#include "GamePlay/BuildSystem/Player/NF_BuildInputComponent.h"

#include "GameFramework/PlayerController.h"

UNF_BuildInputComponent::UNF_BuildInputComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FKey UNF_BuildInputComponent::GetSlotKey(int32 SlotIndex) const
{
	if (SlotIndex == 0)
	{
		return SlotOneKey;
	}
	if (SlotIndex == 1)
	{
		return SlotTwoKey;
	}
	return FKey();
}

bool UNF_BuildInputComponent::SetSlotKey(int32 SlotIndex, FKey Key)
{
	const APlayerController* Controller = Cast<APlayerController>(GetOwner());
	if (!Controller || !Controller->IsLocalController() || SlotIndex < 0 || SlotIndex > 1 ||
		!Key.IsValid() || Key.IsAnalog() || Key.IsModifierKey() || Key == EKeys::Escape ||
		Key == GetSlotKey(1 - SlotIndex))
	{
		return false;
	}
	if (GetSlotKey(SlotIndex) == Key)
	{
		return true;
	}
	if (SlotIndex == 0)
	{
		SlotOneKey = Key;
	}
	else
	{
		SlotTwoKey = Key;
	}
	OnSlotKeysChanged.Broadcast();
	return true;
}
