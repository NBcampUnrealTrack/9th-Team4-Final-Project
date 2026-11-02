// NF_BuildShopViewModel.h

#pragma once

#include "CoreMinimal.h"
#include "GamePlay/BuildSystem/Core/NF_BuildTypes.h"
#include "UI/ViewModel/Base/NF_ViewModelBase.h"
#include "Data/Enum/NF_ItemTypes.h"
#include "GamePlay/BuildSystem/UI/NF_BuildShopViewModelBridge.h"
#include "NF_BuildShopViewModel.generated.h"

USTRUCT(BlueprintType)
struct FNF_BuildShopSlotViewData
{
    GENERATED_BODY()

    // 스킬: ActiveSkill/PassiveSkill 무기: Weapon 장비: Armor
    UPROPERTY(BlueprintReadOnly)
    ENF_ItemType ItemType = ENF_ItemType::Weapon;
    
    // Armor 0=머리, 1=몸, 2=발.
    UPROPERTY(BlueprintReadOnly)
    int32 SlotIndex = 0;

    UPROPERTY(BlueprintReadOnly)
    FText SlotName;

    UPROPERTY(BlueprintReadOnly)
    FName ItemId = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    FText ItemName;

    UPROPERTY(BlueprintReadOnly)
    int32 ItemCost = 0;

    UPROPERTY(BlueprintReadOnly)
    bool bCanChooseSlot = false;
};

USTRUCT(BlueprintType)
struct FNF_BuildShopItemViewData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName ItemId = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    FText DisplayName;

    UPROPERTY(BlueprintReadOnly)
    ENF_ItemType ItemType = ENF_ItemType::Weapon;

    UPROPERTY(BlueprintReadOnly)
    int32 ItemCost = 0;

    // 이 아이템을 선택했을 때 들어갈 슬롯.
    // 액티브 스킬처럼 사용자가 칸을 고르는 경우에는 INDEX_NONE.
    // 장비는 0=머리, 1=몸, 2=발 중 하나.
    UPROPERTY(BlueprintReadOnly)
    int32 TargetSlotIndex = INDEX_NONE;

    // 해당 아이템을 현재 빌드에 넣었을 때의 예상 총 코스트.
    UPROPERTY(BlueprintReadOnly)
    int32 CostAfterSelection = 0;

    UPROPERTY(BlueprintReadOnly)
    bool bCanSelect = false;

    UPROPERTY(BlueprintReadOnly)
    FText CannotSelectReason;
};

USTRUCT(BlueprintType)
struct FNF_BuildShopViewData
{
    GENERATED_BODY()

    // 화면 구성: 스킬 3칸, 무기 1칸, 장비 3칸.
    UPROPERTY(BlueprintReadOnly)
    TArray<FNF_BuildShopSlotViewData> SkillSlots;

    UPROPERTY(BlueprintReadOnly)
    TArray<FNF_BuildShopSlotViewData> WeaponSlots;

    UPROPERTY(BlueprintReadOnly)
    TArray<FNF_BuildShopSlotViewData> EquipmentSlots;

    UPROPERTY(BlueprintReadOnly)
    TArray<FNF_BuildShopItemViewData> CatalogItems;

    // 현재 선택된 빌드의 검증 결과.
    UPROPERTY(BlueprintReadOnly)
    FNF_BuildValidation DraftValidation;

    UPROPERTY(BlueprintReadOnly)
    bool bIsShopOpen = false;

    UPROPERTY(BlueprintReadOnly)
    bool bIsAwaitingBuildResult = false;

    UPROPERTY(BlueprintReadOnly)
    bool bIsChoosingActiveSkillSlot = false;
};

UCLASS(BlueprintType)
class PROJECT_NF_API UNF_BuildShopViewModel : public UNF_ViewModelBase, public INF_BuildShopViewModelBridge
{
    GENERATED_BODY()

public:
    virtual void SetBuildSource(UNF_BuildComponent* InBuild) override;

    UFUNCTION(BlueprintCallable, Category = "NF|Build|UI")
    virtual void RequestClose() override;
    UFUNCTION(BlueprintCallable, Category = "NF|Build|UI")
    void SelectCatalogItem(FName ItemId);
    UFUNCTION(BlueprintCallable, Category = "NF|Build|UI")
    void AssignSelectedActiveSkill(int32 SlotIndex);
    UFUNCTION(BlueprintCallable, Category = "NF|Build|UI")
    void RemoveItem(FName ItemId);
    UFUNCTION(BlueprintCallable, Category = "NF|Build|UI")
    void ApplyBuild();

protected:
    virtual void OnDeinitializeViewModel() override;

private:
    TWeakObjectPtr<UNF_BuildComponent> Build;
    void UnbindBuild();
    void ClearApplyResult();
    UFUNCTION()
    void RefreshFromModel();
    UFUNCTION()
    void HandleShopStateChanged(ENF_BuildShopUIState State);
    UFUNCTION()
    void HandleBuildResult(const FNF_BuildValidation& Validation);

    // 빌드 시스템의 상태가 바뀔 때 UI용 결과를 전달받는 함수
    void UpdateShop(const FNF_BuildShopViewData& NewData);

private:
    UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "BuildShop", meta = (AllowPrivateAccess = "true"))
    FText ApplyResultMessage;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "BuildShop", meta = (AllowPrivateAccess = "true"))
    bool bIsAwaitingBuildResult = false;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "BuildShop", meta = (AllowPrivateAccess = "true"))
    bool bIsChoosingActiveSkillSlot = false;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "BuildShop",meta = (AllowPrivateAccess = "true"))
    TArray<FNF_BuildShopSlotViewData> SkillSlots;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "BuildShop",meta = (AllowPrivateAccess = "true"))
    TArray<FNF_BuildShopSlotViewData> WeaponSlots;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "BuildShop",meta = (AllowPrivateAccess = "true"))
    TArray<FNF_BuildShopSlotViewData> EquipmentSlots;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "BuildShop",meta = (AllowPrivateAccess = "true"))
    TArray<FNF_BuildShopItemViewData> CatalogItems;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "BuildShop",meta = (AllowPrivateAccess = "true"))
    int32 CurrentCost = 0;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "BuildShop",meta = (AllowPrivateAccess = "true"))
    int32 MaxCost = 0;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "BuildShop",meta = (AllowPrivateAccess = "true"))
    bool bCanApplyBuild = false;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "BuildShop",meta = (AllowPrivateAccess = "true"))
    FText ValidationMessage;
};
