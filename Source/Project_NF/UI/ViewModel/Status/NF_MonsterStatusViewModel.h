// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "MVVMViewModelBase.h"
#include "NF_MonsterStatusViewModel.generated.h"

class UAbilitySystemComponent;
struct FOnAttributeChangeData;

UCLASS()
class PROJECT_NF_API UNF_MonsterStatusViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()
	
public:
	void SetObservedUnit(UAbilitySystemComponent* InAbilitySystemComponent,const FText& InUnitName);

	void SetUnitName(const FText& InUnitName);
	
	void ClearObservedUnit();

	UFUNCTION(BlueprintPure, FieldNotify, Category = "Unit|UI")
	float GetHealthPercent() const;

	UFUNCTION(BlueprintPure, FieldNotify, Category = "Unit|UI")
	FText GetUnitName() const;
protected:
	virtual void BeginDestroy() override;

private:
	bool ResolveHealthAttributes();
	void BindAbilitySystemDelegates();
	void UnbindAbilitySystemDelegates();
	void RefreshFromAbilitySystem();

	void HandleHealthAttributeChanged(const FOnAttributeChangeData& Data);

	void UpdateHealth(float NewCurrentHealth,float NewMaxHealth);

	UPROPERTY(BlueprintReadOnly,FieldNotify,Category = "Unit|UI",meta = (AllowPrivateAccess = "true"))
	FText UnitName;

	UPROPERTY(BlueprintReadOnly,FieldNotify,Category = "Unit|UI",meta = (AllowPrivateAccess = "true"))
	float CurrentHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly,FieldNotify,Category = "Unit|UI",meta = (AllowPrivateAccess = "true"))
	float MaxHealth = 0.0f;

	TWeakObjectPtr<UAbilitySystemComponent> ObservedAbilitySystemComponent;

	FGameplayAttribute HealthAttribute;
	FGameplayAttribute MaxHealthAttribute;

	FDelegateHandle HealthChangedDelegateHandle;
	FDelegateHandle MaxHealthChangedDelegateHandle;
};
