// Fill out your copyright notice in the Description page of Project Settings.


#include "NF_MonsterStatusViewModel.h"

#include "GamePlay/Combat/Boss/NF_BossAttributeSet.h"
#include "GAS/Attributes/NF_MonsterAttributeSet.h"

void UNF_MonsterStatusViewModel::SetObservedUnit(
	UAbilitySystemComponent* InAbilitySystemComponent,const FText& InUnitName)
{
	UnbindAbilitySystemDelegates();

	ObservedAbilitySystemComponent = InAbilitySystemComponent;

	UE_MVVM_SET_PROPERTY_VALUE(UnitName, InUnitName);

	if (!ResolveHealthAttributes())
	{
		UpdateHealth(0.0f, 0.0f);
		return;
	}

	BindAbilitySystemDelegates();
	RefreshFromAbilitySystem();
}

void UNF_MonsterStatusViewModel::ClearObservedUnit()
{
	UnbindAbilitySystemDelegates();

	ObservedAbilitySystemComponent.Reset();
	HealthAttribute = FGameplayAttribute();
	MaxHealthAttribute = FGameplayAttribute();

	UE_MVVM_SET_PROPERTY_VALUE(UnitName, FText::GetEmpty());

	UpdateHealth(0.0f, 0.0f);
}

void UNF_MonsterStatusViewModel::BeginDestroy()
{
	UnbindAbilitySystemDelegates();

	Super::BeginDestroy();
}

bool UNF_MonsterStatusViewModel::ResolveHealthAttributes()
{
	UAbilitySystemComponent* ASC =ObservedAbilitySystemComponent.Get();

	if (!IsValid(ASC)) {return false;}

	const FGameplayAttribute MonsterHealthAttribute =UNF_MonsterAttributeSet::GetHealthAttribute();

	if (ASC->HasAttributeSetForAttribute(MonsterHealthAttribute))
	{
		HealthAttribute =UNF_MonsterAttributeSet::GetHealthAttribute();
		MaxHealthAttribute =UNF_MonsterAttributeSet::GetMaxHealthAttribute();

		return true;
	}

	const FGameplayAttribute BossHealthAttribute =UNF_BossAttributeSet::GetHealthAttribute();

	if (ASC->HasAttributeSetForAttribute(BossHealthAttribute))
	{
		HealthAttribute =UNF_BossAttributeSet::GetHealthAttribute();
		MaxHealthAttribute =UNF_BossAttributeSet::GetMaxHealthAttribute();

		return true;
	}

	HealthAttribute = FGameplayAttribute();
	MaxHealthAttribute = FGameplayAttribute();

	return false;
}

void UNF_MonsterStatusViewModel::BindAbilitySystemDelegates()
{
	UAbilitySystemComponent* ASC =ObservedAbilitySystemComponent.Get();

	if (!IsValid(ASC)) return;

	HealthChangedDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(HealthAttribute).AddUObject(
			this,
			&ThisClass::HandleHealthAttributeChanged);

	MaxHealthChangedDelegateHandle =ASC->GetGameplayAttributeValueChangeDelegate(MaxHealthAttribute).AddUObject(
			this,
			&ThisClass::HandleHealthAttributeChanged);
}

void UNF_MonsterStatusViewModel::UnbindAbilitySystemDelegates()
{
	UAbilitySystemComponent* ASC =ObservedAbilitySystemComponent.Get();

	if (IsValid(ASC))
	{
		if (HealthChangedDelegateHandle.IsValid()&& HealthAttribute.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(HealthAttribute).Remove(HealthChangedDelegateHandle);
		}

		if (MaxHealthChangedDelegateHandle.IsValid()&& MaxHealthAttribute.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(MaxHealthAttribute).Remove(MaxHealthChangedDelegateHandle);
		}
	}

	HealthChangedDelegateHandle.Reset();
	MaxHealthChangedDelegateHandle.Reset();
}

void UNF_MonsterStatusViewModel::HandleHealthAttributeChanged(const FOnAttributeChangeData& Data)
{
	RefreshFromAbilitySystem();
}

void UNF_MonsterStatusViewModel::RefreshFromAbilitySystem()
{
	UAbilitySystemComponent* ASC =ObservedAbilitySystemComponent.Get();

	if (!IsValid(ASC) || !HealthAttribute.IsValid() || !MaxHealthAttribute.IsValid())
	{
		UpdateHealth(0.0f, 0.0f);
		return;
	}

	const float NewCurrentHealth =ASC->GetNumericAttribute(HealthAttribute);

	const float NewMaxHealth =ASC->GetNumericAttribute(MaxHealthAttribute);

	UpdateHealth(NewCurrentHealth,NewMaxHealth);
}

void UNF_MonsterStatusViewModel::UpdateHealth(const float NewCurrentHealth,const float NewMaxHealth)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(CurrentHealth,FMath::Max(0.0f, NewCurrentHealth)))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHealthPercent);
	}

	if (UE_MVVM_SET_PROPERTY_VALUE(MaxHealth,FMath::Max(0.0f, NewMaxHealth)))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHealthPercent);
	}
}

float UNF_MonsterStatusViewModel::GetHealthPercent() const
{
	if (MaxHealth <= 0.0f) {return 0.0f;}

	return FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f);
}