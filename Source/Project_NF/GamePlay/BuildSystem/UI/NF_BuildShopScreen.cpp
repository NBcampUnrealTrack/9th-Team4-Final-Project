#include "GamePlay/BuildSystem/UI/NF_BuildShopScreen.h"

#include "GameFramework/Pawn.h"
#include "GamePlay/BuildSystem/Data/NF_BuildItemDefinitionDataAsset.h"
#include "GamePlay/BuildSystem/Player/NF_BuildComponent.h"
#include "UI/ViewModel/BuildShop/NF_BuildShopViewModel.h"
#include "GamePlay/BuildSystem/GamePlay/BuildShop/NF_BuildShopZone.h"

void UNF_BuildShopScreen::NativeConstruct()
{
	Super::NativeConstruct();

	APawn* Pawn = GetOwningPlayerPawn();

	Build = Pawn
		        ? Pawn->FindComponentByClass<UNF_BuildComponent>()
		        : nullptr;

	if (!IsValid(Build))
	{
		return;
	}

	if (!IsValid(ShopViewModel))
	{
		ShopViewModel =
			NewObject<UNF_BuildShopViewModel>(this);
	}

	Build->OnDraftChanged.AddUniqueDynamic(
		this,
		&ThisClass::RefreshViewModel);

	Build->OnBuildChanged.AddUniqueDynamic(
		this,
		&ThisClass::RefreshViewModel);

	Build->OnBuildEntitlementsChanged.AddUniqueDynamic(
		this,
		&ThisClass::RefreshViewModel);

	Build->OnBuildRequestCompleted.AddUniqueDynamic(
		this,
		&ThisClass::HandleBuildResult);

	OnViewModelReady(ShopViewModel);

	// 이벤트 발생 전에 생성된 상태도 처음부터 표시한다.
	RefreshViewModel();
}

void UNF_BuildShopScreen::NativeDestruct()
{
	if (IsValid(Build))
	{
		Build->OnDraftChanged.RemoveDynamic(
			this,
			&ThisClass::RefreshViewModel);

		Build->OnBuildChanged.RemoveDynamic(
			this,
			&ThisClass::RefreshViewModel);

		Build->OnBuildEntitlementsChanged.RemoveDynamic(
			this,
			&ThisClass::RefreshViewModel);

		Build->OnBuildRequestCompleted.RemoveDynamic(
			this,
			&ThisClass::HandleBuildResult);
	}

	Build = nullptr;

	Super::NativeDestruct();
}

void UNF_BuildShopScreen::HandleBuildResult(
	const FNF_BuildValidation& Validation)
{
	RefreshViewModel();
	OnBuildResult(Validation);
}

void UNF_BuildShopScreen::RefreshViewModel()
{
	if (!IsValid(Build) || !IsValid(ShopViewModel))
	{
		return;
	}

	FNF_BuildShopViewData Data;

	Data.bIsShopOpen =
		IsValid(Build->GetCurrentShop());

	Data.bIsAwaitingBuildResult =
		Build->IsAwaitingBuildResult();

	Data.bIsChoosingActiveSkillSlot =
		!Build->GetPendingActiveSkill().IsNone();

	const TArray<FNF_BuildSelection> Draft =
		Build->GetDraftBuild();

	Data.DraftValidation = Build->PreviewBuild(Draft);

	const bool bCanEdit =
		Data.bIsShopOpen &&
		!Data.bIsAwaitingBuildResult;

	// 검증이 중간에 실패해도 화면에는 전체 선택 코스트를 표시한다.
	// 서버의 검증 결과 자체는 변경하지 않는다.
	int64 DisplayCost = 0;

	for (const FNF_BuildSelection& Selection : Draft)
	{
		const UNF_BuildItemDefinitionDataAsset* Item =
			Build->FindItemDefinition(Selection.ItemId);

		FNF_BuildItemData ItemSource;
		if (IsValid(Item) && Build->ResolveItemData(Item->ItemId, ItemSource))
		{
			DisplayCost += FMath::Max(0, ItemSource.Cost);
		}
	}

	Data.DraftValidation.TotalCost =
		static_cast<int32>(
			FMath::Min<int64>(DisplayCost, MAX_int32));

	auto AddSlot = [this, &Data, bCanEdit](
		TArray<FNF_BuildShopSlotViewData>& Container,
		ENF_ItemType ItemType,
		int32 SlotIndex,
		const TCHAR* SlotName)
	{
		FNF_BuildShopSlotViewData SlotData;

		SlotData.ItemType = ItemType;
		SlotData.SlotIndex = SlotIndex;
		SlotData.SlotName = FText::FromString(SlotName);

		SlotData.bCanChooseSlot =
			bCanEdit &&
			Data.bIsChoosingActiveSkillSlot &&
			ItemType == ENF_ItemType::ActiveSkill;

		const UNF_BuildItemDefinitionDataAsset* Item =
			Build->GetSlotItem(
				ItemType,
				SlotIndex,
				true);

		FNF_BuildItemData ItemSource;
		if (IsValid(Item) && Build->ResolveItemData(Item->ItemId, ItemSource))
		{
			SlotData.ItemId = Item->ItemId;

			SlotData.ItemName = Item->DisplayName.IsEmpty()
				                    ? FText::FromName(Item->ItemId)
				                    : Item->DisplayName;

			SlotData.ItemCost = ItemSource.Cost;
		}

		Container.Add(SlotData);
	};

	AddSlot(
		Data.WeaponSlots,
		ENF_ItemType::Weapon,
		0,
		TEXT("주무기"));

	AddSlot(
		Data.EquipmentSlots,
		ENF_ItemType::Armor,
		0,
		TEXT("머리"));

	AddSlot(
		Data.EquipmentSlots,
		ENF_ItemType::Armor,
		1,
		TEXT("상의"));

	AddSlot(
		Data.EquipmentSlots,
		ENF_ItemType::Armor,
		2,
		TEXT("하의"));

	AddSlot(
		Data.SkillSlots,
		ENF_ItemType::ActiveSkill,
		0,
		TEXT("액티브 1"));

	AddSlot(
		Data.SkillSlots,
		ENF_ItemType::ActiveSkill,
		1,
		TEXT("액티브 2"));

	AddSlot(
		Data.SkillSlots,
		ENF_ItemType::PassiveSkill,
		0,
		TEXT("패시브"));

	const ENF_ItemType Categories[] =
	{
		ENF_ItemType::ActiveSkill,
		ENF_ItemType::PassiveSkill,
		ENF_ItemType::Armor,
		ENF_ItemType::Weapon
	};

	for (ENF_ItemType Category : Categories)
	{
		for (const UNF_BuildItemDefinitionDataAsset* Item :
		     Build->GetCatalogItems(
			     Category,
			     ENF_ArmorSlot::None))
		{
			if (!IsValid(Item))
			{
				continue;
			}

			FNF_BuildItemData ItemSource;
			if (!Build->ResolveItemData(Item->ItemId, ItemSource))
			{
				continue;
			}
			FNF_BuildShopItemViewData ItemData;

			ItemData.ItemId = Item->ItemId;
			ItemData.ItemType = ItemSource.ItemType;
			ItemData.ItemCost = ItemSource.Cost;

			ItemData.DisplayName = Item->DisplayName.IsEmpty()
				                       ? FText::FromName(Item->ItemId)
				                       : Item->DisplayName;

			ItemData.TargetSlotIndex =
				ItemSource.ItemType == ENF_ItemType::ActiveSkill
					? INDEX_NONE
					: FNF_BuildItemSlots::GetDefaultSlotIndex(ItemSource);

			// 편집 가능 여부이며 최종 빌드 적용 가능 여부와 다르다.
			ItemData.bCanSelect = bCanEdit;

			if (!bCanEdit)
			{
				ItemData.CannotSelectReason =
					FText::FromString(
						TEXT("현재 빌드를 편집할 수 없습니다."));
			}

			// 액티브는 배치할 슬롯을 고르기 전까지 예상값을 숨긴다.
			ItemData.CostAfterSelection = INDEX_NONE;

			if (ItemSource.ItemType != ENF_ItemType::ActiveSkill)
			{
				int64 CandidateCost = ItemSource.Cost;

				for (const FNF_BuildSelection& Selection : Draft)
				{
					const UNF_BuildItemDefinitionDataAsset* Existing =
						Build->FindItemDefinition(Selection.ItemId);

					FNF_BuildItemData ExistingSource;
					if (!IsValid(Existing) || !Build->ResolveItemData(Existing->ItemId, ExistingSource))
					{
						continue;
					}

					const bool bReplaced =
						ExistingSource.ItemType == ItemSource.ItemType &&
						Selection.SlotIndex ==
						ItemData.TargetSlotIndex;

					if (!bReplaced)
					{
						CandidateCost += ExistingSource.Cost;
					}
				}

				ItemData.CostAfterSelection =
					static_cast<int32>(
						FMath::Min<int64>(
							CandidateCost,
							MAX_int32));
			}

			Data.CatalogItems.Add(ItemData);
		}
	}

	ShopViewModel->UpdateShop(Data);
}
