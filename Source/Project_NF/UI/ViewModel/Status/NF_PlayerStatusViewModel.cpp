
#include "NF_PlayerStatusViewModel.h"

#include "GameFramework/PlayerState.h"
#include "Player/Character/NF_PlayerLevelAttribute.h"
#include "Player/Character/NF_PlayerStatusAttribute.h"

namespace
{
    FText MakeCurrentMaximumText(float Current, float Maximum)
    {
        const float SafeMaximum = FMath::Max(0.0f, Maximum);
        const float SafeCurrent = FMath::Clamp(Current, 0.0f, SafeMaximum);

        return FText::Format(
            NSLOCTEXT("NFHUD", "CurrentMaximum", "{0} / {1}"),
            FText::AsNumber(FMath::RoundToInt(SafeCurrent)),
            FText::AsNumber(FMath::RoundToInt(SafeMaximum)));
    }

    FText MakeNonNegativeCurrentMaximumText(float Current, float Maximum)
    {
        return FText::Format(
            NSLOCTEXT("NFHUD", "CurrentMaximum", "{0} / {1}"),
            FText::AsNumber(FMath::RoundToInt(FMath::Max(0.0f, Current))),
            FText::AsNumber(FMath::RoundToInt(FMath::Max(0.0f, Maximum))));
    }
}

void UNF_PlayerStatusViewModel::OnInitializeViewModel()
{
    Super::OnInitializeViewModel();
    RefreshPlayerName();
}

void UNF_PlayerStatusViewModel::OnViewModelContextChanged()
{
    Super::OnViewModelContextChanged();
    RefreshPlayerName();

    // 플레이어를 떠나거나 ASC가 아직 준비되지 않은 경우
    if (!IsValid(GetAbilitySystemComponent()))
    {
        UpdateVitals(0.0f, 0.0f, 0.0f, 0.0f);
        RefreshProgressionFromAbilitySystem();
    }
}

void UNF_PlayerStatusViewModel::BindAbilitySystemDelegates()
{
    const auto Delegate =
        FOnGameplayAttributeValueChange::FDelegate::CreateUObject(this,&ThisClass::HandleVitalAttributeChanged);

    BindAttributeChanged(UNF_PlayerStatusAttribute::GetHealthAttribute(),Delegate);

    BindAttributeChanged(UNF_PlayerStatusAttribute::GetMaxHealthAttribute(),Delegate);

    BindAttributeChanged(UNF_PlayerStatusAttribute::GetShieldAttribute(),Delegate);

    BindAttributeChanged(UNF_PlayerStatusAttribute::GetMaxShieldAttribute(),Delegate);

    const auto ProgressionDelegate =
        FOnGameplayAttributeValueChange::FDelegate::CreateUObject(
            this, &ThisClass::HandleProgressionAttributeChanged);

    BindAttributeChanged(UNF_PlayerLevelAttribute::GetCurrentLevelAttribute(), ProgressionDelegate);
    BindAttributeChanged(UNF_PlayerLevelAttribute::GetCurrentExpAttribute(), ProgressionDelegate);
}

void UNF_PlayerStatusViewModel::HandleVitalAttributeChanged(const FOnAttributeChangeData& Data)
{
    RefreshFromAbilitySystem();
}

void UNF_PlayerStatusViewModel::RefreshFromAbilitySystem()
{
    RefreshProgressionFromAbilitySystem();

    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();

    if (!IsValid(ASC) ||!ASC->HasAttributeSetForAttribute(UNF_PlayerStatusAttribute::GetHealthAttribute()))
    {
        UpdateVitals(0.0f, 0.0f, 0.0f, 0.0f);
        return;
    }

    UpdateVitals(
        ASC->GetNumericAttribute(UNF_PlayerStatusAttribute::GetHealthAttribute()),
        ASC->GetNumericAttribute(UNF_PlayerStatusAttribute::GetMaxHealthAttribute()),
        ASC->GetNumericAttribute(UNF_PlayerStatusAttribute::GetShieldAttribute()),
        ASC->GetNumericAttribute(UNF_PlayerStatusAttribute::GetMaxShieldAttribute()));
}

void UNF_PlayerStatusViewModel::HandleProgressionAttributeChanged(const FOnAttributeChangeData&)
{
    RefreshProgressionFromAbilitySystem();
}

void UNF_PlayerStatusViewModel::RefreshProgressionFromAbilitySystem()
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    int32 NewLevel = 0;
    float NewExperience = 0.0f;

    if (IsValid(ASC))
    {
        const FGameplayAttribute LevelAttribute =
            UNF_PlayerLevelAttribute::GetCurrentLevelAttribute();
        const FGameplayAttribute ExperienceAttribute =
            UNF_PlayerLevelAttribute::GetCurrentExpAttribute();

        if (ASC->HasAttributeSetForAttribute(LevelAttribute))
        {
            NewLevel = FMath::Max(1, FMath::FloorToInt(ASC->GetNumericAttribute(LevelAttribute)));
        }

        if (ASC->HasAttributeSetForAttribute(ExperienceAttribute))
        {
            NewExperience = FMath::Max(0.0f, ASC->GetNumericAttribute(ExperienceAttribute));
        }
    }

    if (UE_MVVM_SET_PROPERTY_VALUE(Level, NewLevel))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetLevelText);
    }
    // 필요 경험치는 공용 조회 경로가 준비된 뒤 연결한다.
    if (UE_MVVM_SET_PROPERTY_VALUE(Experience, NewExperience))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetExperiencePercent);
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetExperienceText);
    }
}

void UNF_PlayerStatusViewModel::UpdateVitals(float NewHealth,float NewMaxHealth,float NewShield,float NewMaxShield)
{
    if (UE_MVVM_SET_PROPERTY_VALUE(Health, NewHealth))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHealthPercent);
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHealthText);
    }

    if (UE_MVVM_SET_PROPERTY_VALUE(MaxHealth, NewMaxHealth))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHealthPercent);
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHealthText);
    }

    if (UE_MVVM_SET_PROPERTY_VALUE(Shield, NewShield))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetShieldPercent);
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetShieldText);
    }

    if (UE_MVVM_SET_PROPERTY_VALUE(MaxShield, NewMaxShield))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetShieldPercent);
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetShieldText);
    }
}

void UNF_PlayerStatusViewModel::RefreshPlayerName()
{
    APlayerState* PlayerState =GetViewModelContext().PlayerState.Get();

    const FText NewName = IsValid(PlayerState)? FText::FromString(PlayerState->GetPlayerName()): FText::GetEmpty();

    UE_MVVM_SET_PROPERTY_VALUE(PlayerName, NewName);
}

void UNF_PlayerStatusViewModel::UpdateProgression(int32 NewLevel,float NewExperience,float NewRequiredExperience)
{
    if (UE_MVVM_SET_PROPERTY_VALUE(Level, NewLevel))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetLevelText);
    }

    if (UE_MVVM_SET_PROPERTY_VALUE(Experience, NewExperience))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetExperiencePercent);
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetExperienceText);
    }

    if (UE_MVVM_SET_PROPERTY_VALUE(RequiredExperience, NewRequiredExperience))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetExperiencePercent);
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetExperienceText);
    }
}

void UNF_PlayerStatusViewModel::UpdateCost(int32 NewUsedCost,int32 NewMaxCost)
{
    UE_MVVM_SET_PROPERTY_VALUE(UsedCost, NewUsedCost);
    UE_MVVM_SET_PROPERTY_VALUE(MaxCost, NewMaxCost);
}

float UNF_PlayerStatusViewModel::GetHealthPercent() const
{
    if (MaxHealth > 0.0f)
    {
        return FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f);
    }

    return 0.0f;
}

float UNF_PlayerStatusViewModel::GetShieldPercent() const
{
    if (MaxShield > 0.0f)
    {
        return FMath::Clamp(Shield / MaxShield, 0.0f, 1.0f);
    }

    return 0.0f;
}

float UNF_PlayerStatusViewModel::GetExperiencePercent() const
{
    if (RequiredExperience > 0.0f)
    {
        return FMath::Clamp(Experience / RequiredExperience,0.0f,1.0f);
    }

    return 0.0f;
}

FText UNF_PlayerStatusViewModel::GetHealthText() const
{
    return MakeCurrentMaximumText(Health, MaxHealth);
}

FText UNF_PlayerStatusViewModel::GetShieldText() const
{
    return MakeCurrentMaximumText(Shield, MaxShield);
}

FText UNF_PlayerStatusViewModel::GetLevelText() const
{
    return FText::Format(
        NSLOCTEXT("NFHUD", "Level", "Lv. {0}"),
        FText::AsNumber(FMath::Max(1, Level)));
}

FText UNF_PlayerStatusViewModel::GetExperienceText() const
{
    return MakeNonNegativeCurrentMaximumText(Experience, RequiredExperience);
}
