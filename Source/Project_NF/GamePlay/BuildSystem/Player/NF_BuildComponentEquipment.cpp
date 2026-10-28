#include "GamePlay/BuildSystem/Player/NF_BuildComponent.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GamePlay/BuildSystem/Player/NF_BuildEquipmentAdapter.h"
#include "GamePlay/Combat/Armor/NF_ArmorBase.h"
#include "GamePlay/Combat/Armor/NF_ArmorComponent.h"
#include "GamePlay/Combat/Armor/NF_ArmorDataAsset.h"

// @Owner: 황준현 | 2026-10-28. 방어구를 준비하고, 무기까지 성공한 뒤 이전 액터를 정리한다.
bool UNF_BuildComponent::ApplyEquipmentSelection(const TArray<FNF_BuildSelection>& Selections,
                                                 const TMap<FName, FNF_BuildItemData>& SelectedData)
{
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority()) return false;
	UNF_ArmorComponent* Armors = GetOwner()->FindComponentByClass<UNF_ArmorComponent>();

	struct FArmorChange
	{
		ENF_ArmorSlot Slot;
		ANF_ArmorBase* Previous = nullptr;
		ANF_ArmorBase* Next = nullptr;
	};
	TArray<FArmorChange> Changes;
	TArray<ANF_ArmorBase*> Spawned;
	TMap<FName, TWeakObjectPtr<ANF_ArmorBase>> NextArmors;
	TSet<ENF_ArmorSlot> SelectedSlots;
	auto DiscardSpawned = [&Spawned]()
	{
		for (ANF_ArmorBase* Armor : Spawned)
		{
			if (IsValid(Armor)) Armor->Destroy();
		}
	};

	// 먼저 전 부위의 생성과 DA 적용을 마친다. 이 단계에서는 기존 장비를 건드리지 않는다.
	for (const FNF_BuildSelection& Selection : Selections)
	{
		const FNF_BuildItemData& Data = SelectedData.FindChecked(Selection.ItemId);
		if (Data.ItemType != ENF_ItemType::Armor || !Data.ArmorClass) continue;
		if (!IsValid(Armors) || !FNF_BuildEquipmentAdapter::IsArmorDefinitionCompatible(Data) ||
			SelectedSlots.Contains(Data.ArmorSlot))
		{
			DiscardSpawned();
			return false;
		}
		SelectedSlots.Add(Data.ArmorSlot);
		ANF_ArmorBase* Previous = Armors->GetEquippedArmor(Data.ArmorSlot);
		const TWeakObjectPtr<ANF_ArmorBase>* Tracked = BuildArmors.Find(Selection.ItemId);
		if (Tracked && Tracked->IsValid() && Tracked->Get() == Previous &&
			Previous->GetClass() == Data.ArmorClass.Get() && Previous->GetArmorData() == Data.ArmorData.Get())
		{
			NextArmors.Add(Selection.ItemId, Previous);
			continue;
		}

		FActorSpawnParameters Params;
		Params.Owner = GetOwner();
		Params.Instigator = Cast<APawn>(GetOwner());
		ANF_ArmorBase* Next = GetWorld()->SpawnActor<ANF_ArmorBase>(
			Data.ArmorClass, GetOwner()->GetActorTransform(), Params);
		if (Next) Spawned.Add(Next);
		if (!IsValid(Next) || !Next->SetArmorData(Data.ArmorData.Get()))
		{
			DiscardSpawned();
			return false;
		}
		NextArmors.Add(Selection.ItemId, Next);
		Changes.Add({Data.ArmorSlot, Previous, Next});
	}

	// 선택에서 빠진 부위는 빌드가 추적하는 액터만 해제한다.
	for (const auto& Entry : BuildArmors)
	{
		ANF_ArmorBase* Armor = Entry.Value.Get();
		if (!IsValid(Armor) || SelectedSlots.Contains(Armor->GetArmorSlot())) continue;
		if (IsValid(Armors) && Armors->GetEquippedArmor(Armor->GetArmorSlot()) == Armor)
		{
			Changes.Add({Armor->GetArmorSlot(), Armor, nullptr});
		}
	}

	int32 AppliedCount = 0;
	auto Rollback = [&]()
	{
		for (int32 Index = AppliedCount - 1; Index >= 0; --Index)
		{
			const FArmorChange& Change = Changes[Index];
			if (IsValid(Change.Previous))
			{
				if (!Armors->EquipArmor(Change.Previous))
				{
					UE_LOG(LogTemp, Error, TEXT("빌드 방어구 복원 실패: %s, Slot=%d"),
					       *GetNameSafe(GetOwner()), static_cast<int32>(Change.Slot));
				}
			}
			else if (Armors->GetEquippedArmor(Change.Slot) == Change.Next)
			{
				Armors->UnequipArmor(Change.Slot);
			}
		}
		DiscardSpawned();
	};
	for (const FArmorChange& Change : Changes)
	{
		const bool bApplied = Change.Next
			                      ? Armors->EquipArmor(Change.Next)
			                      : Armors->UnequipArmor(Change.Slot) == Change.Previous;
		if (!bApplied)
		{
			Rollback();
			return false;
		}
		++AppliedCount;
	}

	// 무기는 실패 시 기존 액터를 보존한다. 성공 이후에는 실패 가능한 장착 단계를 두지 않는다.
	if (!ApplyWeaponSelection(Selections, SelectedData))
	{
		Rollback();
		return false;
	}

	TSet<ANF_ArmorBase*> Retired;
	for (const FArmorChange& Change : Changes)
	{
		if (IsValid(Change.Previous)) Retired.Add(Change.Previous);
	}
	for (const auto& Entry : BuildArmors)
	{
		const TWeakObjectPtr<ANF_ArmorBase>* Kept = NextArmors.Find(Entry.Key);
		if (Entry.Value.IsValid() && (!Kept || Kept->Get() != Entry.Value.Get()))
		{
			Retired.Add(Entry.Value.Get());
		}
	}
	BuildArmors = MoveTemp(NextArmors);
	for (ANF_ArmorBase* Armor : Retired) Armor->Destroy();
	return true;
}

void UNF_BuildComponent::ReleaseBuildArmors()
{
	UNF_ArmorComponent* Armors = GetOwner()->FindComponentByClass<UNF_ArmorComponent>();
	for (const auto& Entry : BuildArmors)
	{
		if (ANF_ArmorBase* Armor = Entry.Value.Get())
		{
			if (IsValid(Armors) && Armors->GetEquippedArmor(Armor->GetArmorSlot()) == Armor)
			{
				Armors->UnequipArmor(Armor->GetArmorSlot());
			}
			Armor->Destroy();
		}
	}
	BuildArmors.Reset();
}
