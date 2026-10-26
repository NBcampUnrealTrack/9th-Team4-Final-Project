
#pragma once

#include "CoreMinimal.h"
#include "Data/Enum/NF_ItemTypes.h"
#include "TimerManager.h"
#include "GamePlay/BuildSystem/Core/NF_BuildTypes.h"
#include "UI/ViewModel/Base/NF_GASViewModelBase.h"
#include "NF_SkillSlotViewModel.generated.h"

class UNF_BuildComponent;
class UNF_BuildInputComponent;
class UTexture2D;
class UWorld;

UCLASS(BlueprintType)
class PROJECT_NF_API UNF_SkillSlotViewModel : public UNF_GASViewModelBase
{
    GENERATED_BODY()

public:
    // ActiveSkill: 슬롯 0 또는 1 PassiveSkill: 슬롯 0
    bool ConfigureSlot(ENF_ItemType InSlotType,int32 InSlotIndex);
    
    // 아이콘 데이터 소스가 정해지면 장착된 SkillId에 맞춰 호출.
    void SetSkillIcon(FName InSkillId, UTexture2D* InIcon);

    // 0 = 쿨다운 없음, 1 = 쿨다운 시작.
    UFUNCTION(BlueprintPure, FieldNotify, Category = "SkillSlot")
    float GetCooldownPercent() const;

protected:
    virtual void OnInitializeViewModel() override;
    virtual void OnViewModelContextChanged() override;
    virtual void OnDeinitializeViewModel() override;
    virtual void RefreshFromAbilitySystem() override;

private:
    UFUNCTION()
    void RefreshSlot();

    void BindSources();
    void UnbindSources();
    void StartRefreshTimer();
    void StopRefreshTimer();

    UPROPERTY(BlueprintReadOnly, Category = "SkillSlot",meta = (AllowPrivateAccess = "true"))
    ENF_ItemType SlotType = ENF_ItemType::ActiveSkill;

    UPROPERTY(BlueprintReadOnly, Category = "SkillSlot",meta = (AllowPrivateAccess = "true"))
    int32 SlotIndex = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "SkillSlot",meta = (AllowPrivateAccess = "true"))
    bool bIsPassive = false;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "SkillSlot",meta = (AllowPrivateAccess = "true"))
    bool bIsEquipped = false;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "SkillSlot",meta = (AllowPrivateAccess = "true"))
    FName SkillId = NAME_None;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "SkillSlot",meta = (AllowPrivateAccess = "true"))
    FText SkillName;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "SkillSlot",meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UTexture2D> SkillIcon;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "SkillSlot",meta = (AllowPrivateAccess = "true"))
    FText ShortcutText;

    // 액티브 스킬의 수동 사용 가능 여부.
    UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "SkillSlot",meta = (AllowPrivateAccess = "true"))
    bool bCanUse = false;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "SkillSlot",meta = (AllowPrivateAccess = "true"))
    float CooldownRemainingSeconds = 0.0f;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "SkillSlot",meta = (AllowPrivateAccess = "true"))
    float CooldownDurationSeconds = 0.0f;

    TWeakObjectPtr<UNF_BuildComponent> BuildComponent;
    TWeakObjectPtr<UNF_BuildInputComponent> BuildInputComponent;
    TWeakObjectPtr<UWorld> TimerWorld;
    FTimerHandle RefreshTimerHandle;
};