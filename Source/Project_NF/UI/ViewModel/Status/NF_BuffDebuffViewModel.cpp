
#include "NF_BuffDebuffViewModel.h"

#include "AbilitySystemComponent.h"

void UNF_BuffDebuffViewModel::BindAbilitySystemDelegates()
{
    const auto Delegate =
        FOnGameplayEffectTagCountChanged::FDelegate::CreateUObject(this, &ThisClass::HandleEffectTagChanged);

    BindGameplayTagChanged(KeyHolderTag, Delegate);
    BindGameplayTagChanged(ShieldPenaltyTag, Delegate);
}

void UNF_BuffDebuffViewModel::HandleEffectTagChanged(const FGameplayTag ,int32 )
{
    RefreshFromAbilitySystem();
}

void UNF_BuffDebuffViewModel::RefreshFromAbilitySystem()
{
    const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();

    bool bNewHasKeyBuff = false;
    bool bNewHasShieldPenalty = false;

    if (IsValid(ASC))
    {
        if (KeyHolderTag.IsValid())
        {
            bNewHasKeyBuff = ASC->HasMatchingGameplayTag(KeyHolderTag);
        }

        if (ShieldPenaltyTag.IsValid())
        {
            bNewHasShieldPenalty = ASC->HasMatchingGameplayTag(ShieldPenaltyTag);
        }
    }

    UE_MVVM_SET_PROPERTY_VALUE(bHasKeyBuff, bNewHasKeyBuff);
    UE_MVVM_SET_PROPERTY_VALUE(bHasMaxShieldPenalty, bNewHasShieldPenalty);
    
}
