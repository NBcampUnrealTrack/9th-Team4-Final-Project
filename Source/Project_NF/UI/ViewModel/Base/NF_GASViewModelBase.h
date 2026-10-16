// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "NF_ViewModelBase.h"
#include "NF_GASViewModelBase.generated.h"

class UAbilitySystemComponent;

struct FNF_AttributeViewModelBinding
{
	FGameplayAttribute Attribute;
	FDelegateHandle DelegateHandle;
};

struct FNF_GameplayTagViewModelBinding
{
	FGameplayTag GameplayTag;
	EGameplayTagEventType::Type EventType = EGameplayTagEventType::NewOrRemoved;

	FDelegateHandle DelegateHandle;
};

//GAS 데이터를 사용하는 ViewModel의 base
UCLASS()
class PROJECT_NF_API UNF_GASViewModelBase : public UNF_ViewModelBase
{
	GENERATED_BODY()
	
protected:
	virtual void OnInitializeViewModel() override;
	virtual void OnViewModelContextChanged() override;
	virtual void OnDeinitializeViewModel() override;

	//자식에서 구현
	virtual void BindAbilitySystemDelegates() {};

	//자식에서 구현
	virtual void RefreshFromAbilitySystem() {};

	UAbilitySystemComponent* GetAbilitySystemComponent() const
	{
		return AbilitySystemComponent.Get();
	}

	void BindAttributeChanged(
		const FGameplayAttribute& Attribute,
		const FOnGameplayAttributeValueChange::FDelegate& Delegate);

	void BindGameplayTagChanged(
		const FGameplayTag& GameplayTag,
		const FOnGameplayEffectTagCountChanged::FDelegate& Delegate,
		EGameplayTagEventType::Type EventType =
			EGameplayTagEventType::NewOrRemoved);

private:
	void RebindAbilitySystem();
	void UnbindAbilitySystemDelegates();

	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	TArray<FNF_AttributeViewModelBinding> AttributeBindings;
	TArray<FNF_GameplayTagViewModelBinding> GameplayTagBindings;
};
