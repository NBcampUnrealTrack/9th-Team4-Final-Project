#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "NF_DeployableHealthAttribute.generated.h"

// @Owner: 스킬 담당 (구현: Codex) | 2026-10-14
// 목적: 설치물 전용 GAS 체력. 캐릭터 사망/기절 처리와 분리한다.
UCLASS()
class PROJECT_NF_API UNF_DeployableHealthAttribute : public UAttributeSet
{
	GENERATED_BODY()
public:
	UNF_DeployableHealthAttribute();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "NF|Deployable Health")
	FGameplayAttributeData Health;
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UNF_DeployableHealthAttribute, Health)
	GAMEPLAYATTRIBUTE_VALUE_GETTER(Health)
	GAMEPLAYATTRIBUTE_VALUE_SETTER(Health)
	GAMEPLAYATTRIBUTE_VALUE_INITTER(Health)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "NF|Deployable Health")
	FGameplayAttributeData MaxHealth;
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UNF_DeployableHealthAttribute, MaxHealth)
	GAMEPLAYATTRIBUTE_VALUE_GETTER(MaxHealth)
	GAMEPLAYATTRIBUTE_VALUE_SETTER(MaxHealth)
	GAMEPLAYATTRIBUTE_VALUE_INITTER(MaxHealth)

	// 피해 GE의 양수 피해량을 이 메타 Attribute에 더한다. 소비 후 0으로 초기화한다.
	UPROPERTY(BlueprintReadOnly, Category = "NF|Deployable Health")
	FGameplayAttributeData IncomingDamage;
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UNF_DeployableHealthAttribute, IncomingDamage)
	GAMEPLAYATTRIBUTE_VALUE_GETTER(IncomingDamage)
	GAMEPLAYATTRIBUTE_VALUE_SETTER(IncomingDamage)
	GAMEPLAYATTRIBUTE_VALUE_INITTER(IncomingDamage)
private:
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);
};
