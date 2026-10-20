// Fill out your copyright notice in the Description page of Project Settings.


#include "NF_GASViewModelBase.h"

#include "AbilitySystemComponent.h"

void UNF_GASViewModelBase::OnInitializeViewModel()
{
    Super::OnInitializeViewModel();

    RebindAbilitySystem();
}

void UNF_GASViewModelBase::OnViewModelContextChanged()
{
    Super::OnViewModelContextChanged();

    RebindAbilitySystem();
}

void UNF_GASViewModelBase::OnDeinitializeViewModel()
{
    UnbindAbilitySystemDelegates();
    AbilitySystemComponent.Reset();

    Super::OnDeinitializeViewModel();
}

void UNF_GASViewModelBase::RebindAbilitySystem()
{
    UAbilitySystemComponent* NewAbilitySystemComponent = GetViewModelContext().AbilitySystemComponent.Get();

    // 같은 ASC를 계속 사용하는 경우에는 Delegate를 재등록하지 않고
    // 현재 값만 다시 반영한다.
    if (IsValid(NewAbilitySystemComponent) && AbilitySystemComponent.Get() == NewAbilitySystemComponent)
    {
        RefreshFromAbilitySystem();
        return;
    }

    // 이전 ASC에 등록된 Delegate 제거
    UnbindAbilitySystemDelegates();

    AbilitySystemComponent = NewAbilitySystemComponent;

    if (!AbilitySystemComponent.IsValid()) return;

    // 자식 ViewModel이 필요한 Attribute와 Tag를 등록
    BindAbilitySystemDelegates();

    // 등록한 뒤 현재 상태를 UI에 즉시 반영
    RefreshFromAbilitySystem();
}

void UNF_GASViewModelBase::BindAttributeChanged(
    const FGameplayAttribute& Attribute,
    const FOnGameplayAttributeValueChange::FDelegate& Delegate)
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();

    if (!IsValid(ASC)) return;
    if (!Attribute.IsValid()) return;
    if(!Delegate.IsBound()) return;


    FDelegateHandle DelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(Attribute).Add(Delegate);

    if (!DelegateHandle.IsValid()) return;

    FNF_AttributeViewModelBinding& Binding = AttributeBindings.AddDefaulted_GetRef();

    Binding.Attribute = Attribute;
    Binding.DelegateHandle = DelegateHandle;
}

void UNF_GASViewModelBase::BindGameplayTagChanged(
    const FGameplayTag& GameplayTag,
    const FOnGameplayEffectTagCountChanged::FDelegate& Delegate,
    EGameplayTagEventType::Type EventType)
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();

    if (!IsValid(ASC)) return;
    if (!GameplayTag.IsValid()) return;
    if(!Delegate.IsBound()) return;

    FDelegateHandle DelegateHandle =ASC->RegisterGameplayTagEvent(GameplayTag, EventType).Add(Delegate);

    if (!DelegateHandle.IsValid()) return;

    FNF_GameplayTagViewModelBinding& Binding = GameplayTagBindings.AddDefaulted_GetRef();

    Binding.GameplayTag = GameplayTag;
    Binding.EventType = EventType;
    Binding.DelegateHandle = DelegateHandle;
}

void UNF_GASViewModelBase::UnbindAbilitySystemDelegates()
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();

    if (IsValid(ASC))
    {
        for (const FNF_AttributeViewModelBinding& Binding: AttributeBindings)
        {
            if (!Binding.Attribute.IsValid()) continue;
            if (!Binding.DelegateHandle.IsValid()) continue;

            ASC->GetGameplayAttributeValueChangeDelegate(Binding.Attribute).Remove(Binding.DelegateHandle);
        }

        for (const FNF_GameplayTagViewModelBinding& Binding: GameplayTagBindings)
        {
            if (!Binding.GameplayTag.IsValid()) continue;
            if (!Binding.DelegateHandle.IsValid()) continue;

            ASC->RegisterGameplayTagEvent(Binding.GameplayTag,Binding.EventType).Remove(Binding.DelegateHandle);
        }
    }

    // ASC가 이미 파괴된 경우에도 저장된 Handle은 비운다.
    AttributeBindings.Reset();
    GameplayTagBindings.Reset();
}