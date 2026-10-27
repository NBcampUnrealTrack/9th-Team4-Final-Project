#include "GamePlay/ConsumableItems/NF_ConsumableComponent.h"

#include "GamePlay/ConsumableItems/Data/NF_ConsumableItemDataAsset.h"
#include "GamePlay/ConsumableItems/NF_ConsumableItem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Project_NF.h"

UNF_ConsumableComponent::UNF_ConsumableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	Slots.SetNum(SlotCount);
}

bool UNF_ConsumableComponent::GetSlot(int32 SlotIndex, FNF_ConsumableSlot& OutSlot) const
{
	OutSlot = FNF_ConsumableSlot();
	if (!Slots.IsValidIndex(SlotIndex)) { return false; }
	OutSlot = Slots[SlotIndex];
	return true;
}

bool UNF_ConsumableComponent::SetSlot(int32 SlotIndex, UNF_ConsumableItemDataAsset* ItemData, int32 Quantity)
{
	if (bChangingSlots || !IsValid(GetOwner()) || !GetOwner()->HasAuthority()
		|| !Slots.IsValidIndex(SlotIndex) || Quantity < 0) { return false; }
	if ((ItemData == nullptr && Quantity != 0) || (ItemData != nullptr && (!IsValid(ItemData) || Quantity == 0))) { return false; }
	FNF_ConsumableSlot& Slot = Slots[SlotIndex];
	if (Slot.ItemData == ItemData && Slot.Quantity == Quantity) { return true; }
	TGuardValue<bool> ChangeGuard(bChangingSlots, true);
	Slot.ItemData = ItemData;
	Slot.Quantity = Quantity;
	GetOwner()->ForceNetUpdate();
	OnSlotsChanged.Broadcast();
	return true;
}

bool UNF_ConsumableComponent::TryAddItem(UNF_ConsumableItemDataAsset* ItemData, int32 Quantity)
{
	if (bChangingSlots || !IsValid(GetOwner()) || GetOwner()->IsActorBeingDestroyed()
		|| !GetOwner()->HasAuthority() || !IsValid(ItemData) || Quantity <= 0) { return false; }
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		const FNF_ConsumableSlot& Slot = Slots[Index];
		if (Slot.ItemData == ItemData && Slot.Quantity > 0)
		{
			if (Quantity > MAX_int32 - Slot.Quantity) { return false; }
			return SetSlot(Index, ItemData, Slot.Quantity + Quantity);
		}
	}
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		if (Slots[Index].ItemData == nullptr && Slots[Index].Quantity == 0)
		{
			return SetSlot(Index, ItemData, Quantity);
		}
	}
	return false;
}

bool UNF_ConsumableComponent::RemoveFromSlot(int32 SlotIndex, int32 Quantity)
{
	if (Quantity <= 0 || !Slots.IsValidIndex(SlotIndex)) { return false; }
	const FNF_ConsumableSlot& Slot = Slots[SlotIndex];
	if (!IsValid(Slot.ItemData.Get()) || Slot.Quantity < Quantity) { return false; }
	const int32 Remaining = Slot.Quantity - Quantity;
	return SetSlot(SlotIndex, Remaining > 0 ? Slot.ItemData.Get() : nullptr, Remaining);
}

bool UNF_ConsumableComponent::ClearSlot(int32 SlotIndex)
{
	return SetSlot(SlotIndex, nullptr, 0);
}

void UNF_ConsumableComponent::OnRep_Slots()
{
	OnSlotsChanged.Broadcast();
}

void UNF_ConsumableComponent::NF_UseItem_Implementation(int32 SlotIndex)
{
	if (!Slots.IsValidIndex(SlotIndex) || !IsValid(GetOwner())) { return; }
	if (GetOwner()->HasAuthority())
	{
		TryUseItem(SlotIndex);
	}
	else if (const APawn* Pawn = Cast<APawn>(GetOwner()); Pawn && Pawn->IsLocallyControlled())
	{
		ServerUseItem(SlotIndex);
	}
}

void UNF_ConsumableComponent::ServerUseItem_Implementation(int32 SlotIndex)
{
	TryUseItem(SlotIndex);
}

bool UNF_ConsumableComponent::TryUseItem(int32 SlotIndex)
{
	AActor* User = GetOwner();
	if (bChangingSlots || !IsValid(User) || User->IsActorBeingDestroyed() || !User->HasAuthority()
		|| !GetWorld() || !Slots.IsValidIndex(SlotIndex)) { return false; }
	const FNF_ConsumableSlot Before = Slots[SlotIndex];
	const auto* Data = Before.ItemData.Get();
	if (!IsValid(Data) || Before.Quantity <= 0)
	{
		UE_LOG(LogProject_NF, Log, TEXT("[Consumable] 사용 취소: 빈 슬롯 | 캐릭터=%s | 슬롯=%d | 수량=%d"),
			*GetNameSafe(User), SlotIndex, Before.Quantity);
		return false;
	}
	if (!Data->ItemClass
		|| !Data->ItemClass->IsChildOf(ANF_ConsumableItem::StaticClass())
		|| Data->ItemClass->HasAnyClassFlags(CLASS_Abstract)) { return false; }

	// Lock both effect callbacks and quantity changes so a reentrant use cannot spend the same item twice.
	TGuardValue<bool> ChangeGuard(bChangingSlots, true);
	FActorSpawnParameters Params;
	Params.Owner = User;
	Params.Instigator = Cast<APawn>(User);
	Params.ObjectFlags |= RF_Transient;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* Item = GetWorld()->SpawnActor<ANF_ConsumableItem>(Data->ItemClass.Get(), User->GetActorTransform(), Params);
	if (!IsValid(Item)) { return false; }
	const bool bUsed = Item->TryUse(User);
	if (IsValid(Item)) { Item->Destroy(); }
	if (!bUsed) { return false; }
	if (!IsValid(User) || User->IsActorBeingDestroyed() || !IsValid(this)) { return false; }

	FNF_ConsumableSlot& Slot = Slots[SlotIndex];
	// Internal storage cannot be replaced during TryUse; verify before charging nevertheless.
	if (Slot.ItemData != Before.ItemData || Slot.Quantity != Before.Quantity) { return false; }
	--Slot.Quantity;
	if (Slot.Quantity == 0) { Slot.ItemData = nullptr; }
	User->ForceNetUpdate();
	UE_LOG(LogProject_NF, Log, TEXT("[Consumable] 사용 성공 | 캐릭터=%s | 슬롯=%d | 아이템=%s | 남은수량=%d"),
		*GetNameSafe(User), SlotIndex, *GetNameSafe(Data), Slot.Quantity);
	OnSlotsChanged.Broadcast();
	return true;
}

void UNF_ConsumableComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UNF_ConsumableComponent, Slots, COND_OwnerOnly);
}
