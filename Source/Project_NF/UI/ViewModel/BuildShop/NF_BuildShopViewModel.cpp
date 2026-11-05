// NF_BuildShopViewModel.cpp

#include "NF_BuildShopViewModel.h"

#include "GamePlay/BuildSystem/Core/NF_BuildTypes.h"
#include "GamePlay/BuildSystem/Player/NF_BuildComponent.h"
#include "GamePlay/BuildSystem/Data/NF_BuildItemDefinitionDataAsset.h"
#include "GamePlay/BuildSystem/Data/NF_BuildRules.h"
#include "GamePlay/BuildSystem/GamePlay/BuildShop/NF_BuildShopZone.h"
#include "GamePlay/Combat/Skill/Data/NF_SkillDataAsset.h"
#include "Engine/Texture2D.h"

void UNF_BuildShopViewModel::UpdateShop(const FNF_BuildShopViewData& NewData)
{
	UE_MVVM_SET_PROPERTY_VALUE(bIsAwaitingBuildResult, NewData.bIsAwaitingBuildResult);
	UE_MVVM_SET_PROPERTY_VALUE(bIsChoosingActiveSkillSlot, NewData.bIsChoosingActiveSkillSlot);
	SkillSlots = NewData.SkillSlots;
	WeaponSlots = NewData.WeaponSlots;
	EquipmentSlots = NewData.EquipmentSlots;
	CatalogItems = NewData.CatalogItems;

	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SkillSlots);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(WeaponSlots);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(EquipmentSlots);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CatalogItems);

	UE_MVVM_SET_PROPERTY_VALUE(CurrentCost, NewData.DraftValidation.TotalCost);

	UE_MVVM_SET_PROPERTY_VALUE(MaxCost, NewData.DraftValidation.CostLimit);

	UE_MVVM_SET_PROPERTY_VALUE(ValidationMessage, NewData.DraftValidation.Message);

	bool bNewCanApplyBuild = false;

	if (NewData.bIsShopOpen &&
		!NewData.bIsAwaitingBuildResult &&
		!NewData.bIsChoosingActiveSkillSlot &&
		NewData.DraftValidation.Result == ENF_BuildResult::Success)
	{
		bNewCanApplyBuild = true;
	}

	UE_MVVM_SET_PROPERTY_VALUE(bCanApplyBuild, bNewCanApplyBuild);
}

void UNF_BuildShopViewModel::SetBuildSource(UNF_BuildComponent* InBuild)
{
	if (IsValid(InBuild) && Build.Get() == InBuild)
	{
		RefreshFromModel();
		return;
	}
	UnbindBuild();
	ClearApplyResult();
	Build = InBuild;
	if (UNF_BuildComponent* Source = Build.Get())
	{
		Source->OnDraftChanged.AddUniqueDynamic(this, &ThisClass::RefreshFromModel);
		Source->OnBuildChanged.AddUniqueDynamic(this, &ThisClass::RefreshFromModel);
		Source->OnBuildShopStateChanged.AddUniqueDynamic(this, &ThisClass::HandleShopStateChanged);
		Source->OnBuildRequestCompleted.AddUniqueDynamic(this, &ThisClass::HandleBuildResult);
	}
	RefreshFromModel();
}

void UNF_BuildShopViewModel::UnbindBuild()
{
	if (UNF_BuildComponent* Source = Build.Get())
	{
		Source->OnDraftChanged.RemoveDynamic(this, &ThisClass::RefreshFromModel);
		Source->OnBuildChanged.RemoveDynamic(this, &ThisClass::RefreshFromModel);
		Source->OnBuildShopStateChanged.RemoveDynamic(this, &ThisClass::HandleShopStateChanged);
		Source->OnBuildRequestCompleted.RemoveDynamic(this, &ThisClass::HandleBuildResult);
	}
	Build.Reset();
}

void UNF_BuildShopViewModel::OnDeinitializeViewModel()
{
	SetBuildSource(nullptr);
	Super::OnDeinitializeViewModel();
}

void UNF_BuildShopViewModel::ClearApplyResult()
{
	UE_MVVM_SET_PROPERTY_VALUE(ApplyResultMessage, FText::GetEmpty());
}

void UNF_BuildShopViewModel::HandleShopStateChanged(ENF_BuildShopUIState State)
{
	ClearApplyResult();
	RefreshFromModel();
}

void UNF_BuildShopViewModel::HandleBuildResult(const FNF_BuildValidation& Validation)
{
	if (!Build.IsValid())
	{
		return;
	}
	UE_MVVM_SET_PROPERTY_VALUE(ApplyResultMessage, Validation.Message);
}

void UNF_BuildShopViewModel::RequestClose()
{
	if (UNF_BuildComponent* Source = Build.Get())
	{
		Source->CloseBuildSession();
	}
}

void UNF_BuildShopViewModel::SelectCatalogItem(FName ItemId)
{
	if (UNF_BuildComponent* Source = Build.Get())
	{
		ClearApplyResult();

		// 이미 선택 표시된 항목(또는 칸 선택 대기 중인 스킬)을 다시 누르면 선택을 취소한다.
		const bool bInDraft = Source->GetDraftBuild().ContainsByPredicate([ItemId](const FNF_BuildSelection& Selection)
		{
			return Selection.ItemId == ItemId;
		});

		if ((Source->GetPickedItemIds().Contains(ItemId) && bInDraft) || Source->GetPendingActiveSkill() == ItemId)
		{
			Source->RevertDraftItem(ItemId);
			return;
		}

		Source->SelectDraftItem(ItemId);
	}
}

void UNF_BuildShopViewModel::AssignSelectedActiveSkill(int32 SlotIndex)
{
	if (UNF_BuildComponent* Source = Build.Get())
	{
		ClearApplyResult();
		Source->AssignSelectedActiveSkill(SlotIndex);
	}
}

void UNF_BuildShopViewModel::RemoveItem(FName ItemId)
{
	if (UNF_BuildComponent* Source = Build.Get())
	{
		ClearApplyResult();
		Source->RemoveDraftItem(ItemId);
	}
}

void UNF_BuildShopViewModel::ApplyBuild()
{
	RefreshFromModel();
	if (UNF_BuildComponent* Source = Build.Get())
	{
		if (bCanApplyBuild)
		{
			ClearApplyResult();
			Source->ApplyDraftBuild();
		}
	}
}

void UNF_BuildShopViewModel::RefreshFromModel()
{
	UNF_BuildComponent* Source = Build.Get();
	FNF_BuildShopViewData Data;
	if (!Source)
	{
		ClearApplyResult();
		UpdateShop(Data);
		return;
	}
	Data.bIsShopOpen = IsValid(Source->GetCurrentShop());
	Data.bIsAwaitingBuildResult = Source->IsAwaitingBuildResult();
	Data.bIsChoosingActiveSkillSlot = !Source->GetPendingActiveSkill().IsNone();
	const TArray<FNF_BuildSelection> Draft = Source->GetDraftBuild();
	Data.DraftValidation = Source->PreviewBuild(Draft);
	const bool bCanEdit = Data.bIsShopOpen && !Data.bIsAwaitingBuildResult;

	// 이번 상점 방문에서 고른 항목. 선택 표시와 코스트 계산에 쓴다.
	const TSet<FName> PickedItemIds(Source->GetPickedItemIds());
	TSet<FName> EquippedItemIds;

	// 장착 중이고 이번 방문에서 다시 고르지 않은 항목은 코스트에 넣지 않는다. 상점을 열 때마다 0부터 시작한다.
	TSet<FName> FreeItemIds;
	for (const FNF_BuildSelection& Selection : Source->GetCurrentBuild())
	{
		EquippedItemIds.Add(Selection.ItemId);
		if (!PickedItemIds.Contains(Selection.ItemId))
		{
			FreeItemIds.Add(Selection.ItemId);
		}
	}
	// 검증이 중간에 실패해도 전체 선택 코스트를 표시한다.
	int64 DisplayCost = 0;
	for (const FNF_BuildSelection& Selection : Draft)
	{
		const UNF_BuildItemDefinitionDataAsset* Item = Source->FindItemDefinition(Selection.ItemId);
		FNF_BuildItemData ItemSource;
		if (IsValid(Item) && !FreeItemIds.Contains(Item->ItemId) &&
			Source->ResolveItemData(Item->ItemId, ItemSource))
		{
			DisplayCost += FMath::Max(0, ItemSource.Cost);
		}
	}
	Data.DraftValidation.TotalCost = static_cast<int32>(FMath::Min<int64>(DisplayCost, MAX_int32));

	auto AddSlot = [Source, &Data, bCanEdit](TArray<FNF_BuildShopSlotViewData>& Container,
	                                         ENF_ItemType ItemType, int32 SlotIndex, const TCHAR* SlotName)
	{
		FNF_BuildShopSlotViewData SlotData;
		SlotData.ItemType = ItemType;
		SlotData.SlotIndex = SlotIndex;
		SlotData.SlotName = FText::FromString(SlotName);
		SlotData.bCanChooseSlot = bCanEdit && Data.bIsChoosingActiveSkillSlot &&
			ItemType == ENF_ItemType::ActiveSkill;
		const UNF_BuildItemDefinitionDataAsset* Item = Source->GetSlotItem(ItemType, SlotIndex, true);
		FNF_BuildItemData ItemSource;
		if (IsValid(Item) && Source->ResolveItemData(Item->ItemId, ItemSource))
		{
			SlotData.ItemId = Item->ItemId;
			SlotData.ItemName = Item->DisplayName;
			if (SlotData.ItemName.IsEmpty())
			{
				SlotData.ItemName = FText::FromName(Item->ItemId);
			}
			SlotData.ItemCost = ItemSource.Cost;
		}
		Container.Add(SlotData);
	};

	AddSlot(Data.WeaponSlots, ENF_ItemType::Weapon, 0, TEXT("주무기"));
	AddSlot(Data.EquipmentSlots, ENF_ItemType::Armor, 0, TEXT("머리"));
	AddSlot(Data.EquipmentSlots, ENF_ItemType::Armor, 1, TEXT("상의"));
	AddSlot(Data.EquipmentSlots, ENF_ItemType::Armor, 2, TEXT("하의"));
	AddSlot(Data.SkillSlots, ENF_ItemType::ActiveSkill, 0, TEXT("액티브 1"));
	AddSlot(Data.SkillSlots, ENF_ItemType::ActiveSkill, 1, TEXT("액티브 2"));
	AddSlot(Data.SkillSlots, ENF_ItemType::PassiveSkill, 0, TEXT("패시브"));

	TSet<FName> DraftItemIds;
	for (const FNF_BuildSelection& Selection : Draft)
	{
		DraftItemIds.Add(Selection.ItemId);
	}

	const ENF_ItemType Categories[] = {
		ENF_ItemType::ActiveSkill, ENF_ItemType::PassiveSkill, ENF_ItemType::Armor, ENF_ItemType::Weapon
	};
	for (ENF_ItemType Category : Categories)
	{
		for (const UNF_BuildItemDefinitionDataAsset* Item :
		     Source->GetCatalogItems(Category, ENF_ArmorSlot::None))
		{
			if (!IsValid(Item))
			{
				continue;
			}
			FNF_BuildItemData ItemSource;
			if (!Source->ResolveItemData(Item->ItemId, ItemSource))
			{
				continue;
			}
			FNF_BuildShopItemViewData ItemData;
			ItemData.ItemId = Item->ItemId;
			ItemData.ItemType = ItemSource.ItemType;
			ItemData.ItemCost = ItemSource.Cost;

			// 스킬은 원본 NF_SkillDataAsset의 아이콘을 사용한다. 그 외 원본은 아이콘 없음.
			if (const UNF_BuildRules* Rules = Source->GetRules())
			{
				if (const TObjectPtr<UDataAsset>* IconSource = Rules->ItemSources.Find(Item->ItemId))
				{
					if (const UNF_SkillDataAsset* SkillData = Cast<UNF_SkillDataAsset>(IconSource->Get()))
					{
						ItemData.Icon = SkillData->GetSkillIcon();
					}
				}
			}

			ItemData.DisplayName = Item->DisplayName;
			if (ItemData.DisplayName.IsEmpty())
			{
				ItemData.DisplayName = FText::FromName(Item->ItemId);
			}
			ItemData.bIsEquipped = EquippedItemIds.Contains(Item->ItemId);
			ItemData.bIsSelected = DraftItemIds.Contains(Item->ItemId) && PickedItemIds.Contains(Item->ItemId);
			ItemData.bCanSelect = bCanEdit;
			if (!bCanEdit)
			{
				ItemData.CannotSelectReason = FText::FromString(TEXT("현재 빌드를 편집할 수 없습니다."));
			}
			ItemData.CostAfterSelection = INDEX_NONE;
			if (ItemSource.ItemType != ENF_ItemType::ActiveSkill)
			{
				ItemData.TargetSlotIndex = FNF_BuildItemSlots::GetDefaultSlotIndex(ItemSource);

				int64 CandidateCost = ItemSource.Cost;
				for (const FNF_BuildSelection& Selection : Draft)
				{
					const UNF_BuildItemDefinitionDataAsset* Existing = Source->FindItemDefinition(Selection.ItemId);
					FNF_BuildItemData ExistingSource;
					if (!IsValid(Existing) || !Source->ResolveItemData(Existing->ItemId, ExistingSource))
					{
						continue;
					}
					const bool bReplaced = ExistingSource.ItemType == ItemSource.ItemType &&
						Selection.SlotIndex == ItemData.TargetSlotIndex;
					if (!bReplaced && Selection.ItemId != Item->ItemId && !FreeItemIds.Contains(Selection.ItemId))
					{
						CandidateCost += ExistingSource.Cost;
					}
				}
				ItemData.CostAfterSelection = static_cast<int32>(FMath::Min<int64>(CandidateCost, MAX_int32));
			}
			Data.CatalogItems.Add(ItemData);
		}
	}
	UpdateShop(Data);
}
