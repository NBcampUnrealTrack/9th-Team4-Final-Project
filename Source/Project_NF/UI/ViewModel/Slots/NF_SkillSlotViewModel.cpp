// NF_SkillSlotViewModel.cpp

#include "NF_SkillSlotViewModel.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameplayAbilitySpec.h"
#include "GamePlay/BuildSystem/Core/NF_BuildItemSource.h"
#include "GamePlay/BuildSystem/Data/NF_BuildItemDefinitionDataAsset.h"
#include "GamePlay/BuildSystem/Player/NF_BuildComponent.h"
#include "GamePlay/BuildSystem/Player/NF_BuildInputComponent.h"
#include "GamePlay/Combat/Skill/Data/NF_SkillDataAsset.h"

namespace
{
    UTexture2D* ResolveSkillIcon(const UNF_BuildComponent* Build, FName ItemId)
    {
        if (!IsValid(Build) || ItemId.IsNone()){return nullptr;}

        const UNF_BuildRules* Rules = Build->GetRules();
        const UNF_SkillDataAsset* SkillData = Cast<UNF_SkillDataAsset>(Build->GetItemSource(ItemId));

        return SkillData->GetSkillIcon();
    }
}

bool UNF_SkillSlotViewModel::ConfigureSlot(ENF_ItemType InSlotType,int32 InSlotIndex)
{
    switch (InSlotType)
    {
    case ENF_ItemType::ActiveSkill:
        if (InSlotIndex < 0 || InSlotIndex > 1) {return false;}
        break;

    case ENF_ItemType::PassiveSkill:
        if (InSlotIndex != 0) {return false;}
        break;

    default:
        return false;
    }

    StopRefreshTimer();

    SlotType = InSlotType;
    SlotIndex = InSlotIndex;

    if (UE_MVVM_SET_PROPERTY_VALUE(bIsPassive,InSlotType == ENF_ItemType::PassiveSkill))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetShortcutVisibility);
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetCooldownText);
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetCooldownVisibility);
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetSkillIconOpacity);
    }

    if (IsViewModelInitialized()) {StartRefreshTimer();}

    RefreshSlot();
    return true;
}

void UNF_SkillSlotViewModel::SetSkillIcon(FName InSkillId,UTexture2D* InIcon)
{
   
    if (SkillId != InSkillId) return;

    if (UE_MVVM_SET_PROPERTY_VALUE(SkillIcon, InIcon))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetSkillIconVisibility);
    }
}

float UNF_SkillSlotViewModel::GetCooldownPercent() const
{
    if (CooldownDurationSeconds <= 0.0f) {return 0.0f;}

    return FMath::Clamp(CooldownRemainingSeconds / CooldownDurationSeconds,0.0f,1.0f);
}

FText UNF_SkillSlotViewModel::GetCooldownText() const
{
    if (!ShouldShowCooldown()) {return FText::GetEmpty();}

    return FText::AsNumber(FMath::CeilToInt(CooldownRemainingSeconds));
}

ESlateVisibility UNF_SkillSlotViewModel::GetSkillIconVisibility() const
{
    if (bIsEquipped && IsValid(SkillIcon)) {return ESlateVisibility::HitTestInvisible;}

    return ESlateVisibility::Hidden;
}

ESlateVisibility UNF_SkillSlotViewModel::GetShortcutVisibility() const
{
    if (bIsEquipped && !bIsPassive) {return ESlateVisibility::HitTestInvisible;}

    return ESlateVisibility::Collapsed;
}

ESlateVisibility UNF_SkillSlotViewModel::GetCooldownVisibility() const
{
    if (ShouldShowCooldown()) {return ESlateVisibility::HitTestInvisible;}

    return ESlateVisibility::Collapsed;
}

float UNF_SkillSlotViewModel::GetSkillIconOpacity() const
{
    if (bIsEquipped && (bIsPassive || bCanUse)) {return 1.0f;}

    return 0.35f;
}

bool UNF_SkillSlotViewModel::ShouldShowCooldown() const
{
    return bIsEquipped && !bIsPassive && CooldownRemainingSeconds > 0.0f;
}

void UNF_SkillSlotViewModel::OnInitializeViewModel()
{
    Super::OnInitializeViewModel();

    BindSources();
    StartRefreshTimer();
    RefreshSlot();
}

void UNF_SkillSlotViewModel::OnViewModelContextChanged()
{
    StopRefreshTimer();
    UnbindSources();

    Super::OnViewModelContextChanged();

    BindSources();
    StartRefreshTimer();
    RefreshSlot();
}

void UNF_SkillSlotViewModel::OnDeinitializeViewModel()
{
    StopRefreshTimer();
    UnbindSources();

    Super::OnDeinitializeViewModel();
}

void UNF_SkillSlotViewModel::RefreshFromAbilitySystem()
{
    RefreshSlot();
}

void UNF_SkillSlotViewModel::BindSources()
{
    APawn* Pawn = GetViewModelContext().Pawn.Get();
    APlayerController* Controller =GetViewModelContext().PlayerController.Get();

    if (IsValid(Pawn))
    {
        BuildComponent =Pawn->FindComponentByClass<UNF_BuildComponent>();
    }

    if (IsValid(Controller))
    {
        BuildInputComponent =Controller->FindComponentByClass<UNF_BuildInputComponent>();
    }

    if (UNF_BuildComponent* Build = BuildComponent.Get())
    {
        Build->OnBuildChanged.AddUniqueDynamic(this, &ThisClass::RefreshSlot);
    }

    if (UNF_BuildInputComponent* Input = BuildInputComponent.Get())
    {
        Input->OnSlotKeysChanged.AddUniqueDynamic(this, &ThisClass::RefreshSlot);
    }
}

void UNF_SkillSlotViewModel::UnbindSources()
{
    if (UNF_BuildComponent* Build = BuildComponent.Get())
    {
        Build->OnBuildChanged.RemoveDynamic(this, &ThisClass::RefreshSlot);
    }

    if (UNF_BuildInputComponent* Input = BuildInputComponent.Get())
    {
        Input->OnSlotKeysChanged.RemoveDynamic(this, &ThisClass::RefreshSlot);
    }

    BuildComponent.Reset();
    BuildInputComponent.Reset();
}

void UNF_SkillSlotViewModel::RefreshSlot()
{
    FName NewSkillId = NAME_None;
    FText NewSkillName;
    FText NewShortcutText;
    UTexture2D* NewSkillIcon = nullptr;

    float NewRemainingSeconds = 0.0f;
    float NewDurationSeconds = 0.0f;
    bool bNewCanUse = false;

    const UNF_BuildComponent* Build = BuildComponent.Get();
    const UNF_BuildInputComponent* Input = BuildInputComponent.Get();
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();

    // 현재 적용된 빌드에서 이 슬롯의 스킬을 찾는다.
    // 상점에서 수정 중인 DraftBuild는 HUD에 표시하지 않는다.
    if (SlotIndex >= 0 && IsValid(Build))
    {
        for (const FNF_BuildSelection& Selection :Build->GetCurrentBuild())
        {
            if (Selection.SlotIndex != SlotIndex)continue;

            const UNF_BuildItemDefinitionDataAsset* Item =Build->FindItemDefinition(Selection.ItemId);

            FNF_BuildItemData ItemData;
            if (!IsValid(Item) || !Build->ResolveItemData(Selection.ItemId, ItemData) ||
                ItemData.ItemType != SlotType)continue;

            NewSkillId = Item->ItemId;
            NewSkillName = Item->DisplayName;
            NewSkillIcon = ResolveSkillIcon(Build, Selection.ItemId);

            if (NewSkillName.IsEmpty())
            {
                NewSkillName = FText::FromName(Item->ItemId);
            }

            break;
        }
    }

    const bool bActiveSlot =SlotType == ENF_ItemType::ActiveSkill;

    // 패시브에는 단축키와 수동 사용 상태를 표시하지 않는다.
    if (bActiveSlot && SlotIndex >= 0 && IsValid(Input))
    {
        const FKey Key = Input->GetSlotKey(SlotIndex);

        if (Key.IsValid())
        {
            NewShortcutText = Key.GetDisplayName();
        }
    }

    // 액티브 스킬만 GAS InputID 1/2에 연결되어 있다.
    if (bActiveSlot &&!NewSkillId.IsNone() &&IsValid(Build) &&IsValid(ASC) &&ASC->AbilityActorInfo.IsValid())
    {
        const int32 ExpectedInputId = SlotIndex + 1;

        for (const FGameplayAbilitySpec& Spec :ASC->GetActivatableAbilities())
        {
            if (Spec.InputID != ExpectedInputId) continue;

            const UGameplayAbility* Ability =Spec.GetPrimaryInstance();

            if (!IsValid(Ability)) {Ability = Spec.Ability;}

            if (!IsValid(Ability)) break;

            Ability->GetCooldownTimeRemainingAndDuration(
                Spec.Handle,ASC->AbilityActorInfo.Get(),NewRemainingSeconds,NewDurationSeconds);

            // 상점 안에서는 입력이 스킬 발동 대신 슬롯 선택에 쓰임.
            if (Build->GetCurrentShop() == nullptr)
            {
                bNewCanUse = Ability->CanActivateAbility(Spec.Handle,ASC->AbilityActorInfo.Get());
            }

            break;
        }
    }

    UE_MVVM_SET_PROPERTY_VALUE(SkillId, NewSkillId);
    UE_MVVM_SET_PROPERTY_VALUE(SkillName, NewSkillName);
    // 빈 슬롯, 원본 누락, 아이콘 미지정 시에도 이전 이미지를 정리한다.
    if (UE_MVVM_SET_PROPERTY_VALUE(SkillIcon, NewSkillIcon))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetSkillIconVisibility);
    }
    if (UE_MVVM_SET_PROPERTY_VALUE(bIsEquipped, !NewSkillId.IsNone()))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetSkillIconVisibility);
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetShortcutVisibility);
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetCooldownText);
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetCooldownVisibility);
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetSkillIconOpacity);
    }
    UE_MVVM_SET_PROPERTY_VALUE(ShortcutText, NewShortcutText);
    if (UE_MVVM_SET_PROPERTY_VALUE(bCanUse, bNewCanUse))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetSkillIconOpacity);
    }

    if (UE_MVVM_SET_PROPERTY_VALUE(CooldownRemainingSeconds,FMath::Max(0.0f, NewRemainingSeconds)))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetCooldownPercent);
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetCooldownText);
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetCooldownVisibility);
    }

    if (UE_MVVM_SET_PROPERTY_VALUE(CooldownDurationSeconds,FMath::Max(0.0f, NewDurationSeconds)))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetCooldownPercent);
    }
}

void UNF_SkillSlotViewModel::StartRefreshTimer()
{
    if (SlotType != ENF_ItemType::ActiveSkill)return;
    if (SlotIndex == INDEX_NONE) return;
    
    APlayerController* Controller =GetViewModelContext().PlayerController.Get();

    if (!IsValid(Controller))return;

    UWorld* World = Controller->GetWorld();

    if (!IsValid(World))return;

    TimerWorld = World;

    World->GetTimerManager().SetTimer(
        RefreshTimerHandle,
        this,
        &ThisClass::RefreshSlot,
        0.1f,
        true);
}

void UNF_SkillSlotViewModel::StopRefreshTimer()
{
    if (UWorld* World = TimerWorld.Get())
    {
        World->GetTimerManager().ClearTimer(RefreshTimerHandle);
    }

    TimerWorld.Reset();
}
