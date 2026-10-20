

#include "GameFramework/Pawn.h"
#include "GamePlay/BuildSystem/Data/NF_BuildItemDefinitionDataAsset.h"
#include "GamePlay/BuildSystem/Player/NF_BuildComponent.h"
#include "UI/ViewModel/BuildShop/NF_BuildShopViewModel.h"
#include "GamePlay/BuildSystem/UI/NF_BuildShopScreen.h"

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

        if (IsValid(Item))
        {
            DisplayCost += FMath::Max(0, Item->Cost);
        }
    }

    Data.DraftValidation.TotalCost =
        static_cast<int32>(
            FMath::Min<int64>(DisplayCost, MAX_int32));

    auto AddSlot = [this, &Data, bCanEdit](
        TArray<FNF_BuildShopSlotViewData>& Container,
        ENF_BuildItemType ItemType,
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
            ItemType == ENF_BuildItemType::ActiveSkill;

        const UNF_BuildItemDefinitionDataAsset* Item =
            Build->GetSlotItem(
                ItemType,
                SlotIndex,
                true);

        if (IsValid(Item))
        {
            SlotData.ItemId = Item->ItemId;

            SlotData.ItemName = Item->DisplayName.IsEmpty()
                ? FText::FromName(Item->ItemId)
                : Item->DisplayName;

            SlotData.ItemCost = Item->Cost;
        }

        Container.Add(SlotData);
    };

    AddSlot(
        Data.WeaponSlots,
        ENF_BuildItemType::Weapon,
        0,
        TEXT("주무기"));

    AddSlot(
        Data.EquipmentSlots,
        ENF_BuildItemType::Armor,
        0,
        TEXT("머리"));

    AddSlot(
        Data.EquipmentSlots,
        ENF_BuildItemType::Armor,
        1,
        TEXT("상의"));

    AddSlot(
        Data.EquipmentSlots,
        ENF_BuildItemType::Armor,
        2,
        TEXT("하의"));

    AddSlot(
        Data.SkillSlots,
        ENF_BuildItemType::ActiveSkill,
        0,
        TEXT("액티브 1"));

    AddSlot(
        Data.SkillSlots,
        ENF_BuildItemType::ActiveSkill,
        1,
        TEXT("액티브 2"));

    AddSlot(
        Data.SkillSlots,
        ENF_BuildItemType::PassiveSkill,
        0,
        TEXT("패시브"));

    const ENF_BuildItemType Categories[] =
    {
        ENF_BuildItemType::ActiveSkill,
        ENF_BuildItemType::PassiveSkill,
        ENF_BuildItemType::Armor,
        ENF_BuildItemType::Weapon
    };

    for (ENF_BuildItemType Category : Categories)
    {
        for (const UNF_BuildItemDefinitionDataAsset* Item :
            Build->GetCatalogItems(
                Category,
                ENF_BuildArmorPart::None))
        {
            if (!IsValid(Item))
            {
                continue;
            }

            FNF_BuildShopItemViewData ItemData;

            ItemData.ItemId = Item->ItemId;
            ItemData.ItemType = Item->ItemType;
            ItemData.ItemCost = Item->Cost;

            ItemData.DisplayName = Item->DisplayName.IsEmpty()
                ? FText::FromName(Item->ItemId)
                : Item->DisplayName;

            ItemData.TargetSlotIndex =
                Item->ItemType == ENF_BuildItemType::ActiveSkill
                    ? INDEX_NONE
                    : Item->GetDefaultSlotIndex();

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

            if (Item->ItemType != ENF_BuildItemType::ActiveSkill)
            {
                int64 CandidateCost = Item->Cost;

                for (const FNF_BuildSelection& Selection : Draft)
                {
                    const UNF_BuildItemDefinitionDataAsset* Existing =
                        Build->FindItemDefinition(Selection.ItemId);

                    if (!IsValid(Existing))
                    {
                        continue;
                    }

                    const bool bReplaced =
                        Existing->ItemType == Item->ItemType &&
                        Selection.SlotIndex ==
                            ItemData.TargetSlotIndex;

                    if (!bReplaced)
                    {
                        CandidateCost += Existing->Cost;
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