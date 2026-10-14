#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "NF_MonsterAttributeSet.generated.h"

#define NF_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class PROJECT_NF_API UNF_MonsterAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	virtual void GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PreAttributeChange(
		const FGameplayAttribute& Attribute,
		float& NewValue) override;
	
	UPROPERTY(
	BlueprintReadOnly,
	ReplicatedUsing = OnRep_Health,
	Category = "NF|Monster|Attributes")
	FGameplayAttributeData Health;
	NF_ATTRIBUTE_ACCESSORS(UNF_MonsterAttributeSet, Health)

	UPROPERTY(
		BlueprintReadOnly,
		ReplicatedUsing = OnRep_MaxHealth,
		Category = "NF|Monster|Attributes")
	FGameplayAttributeData MaxHealth;
	NF_ATTRIBUTE_ACCESSORS(UNF_MonsterAttributeSet, MaxHealth)

	UPROPERTY(
		BlueprintReadOnly,
		ReplicatedUsing = OnRep_AttackPower,
		Category = "NF|Monster|Attributes")
	FGameplayAttributeData AttackPower;
	NF_ATTRIBUTE_ACCESSORS(UNF_MonsterAttributeSet, AttackPower)

	UPROPERTY(
		BlueprintReadOnly,
		ReplicatedUsing = OnRep_MoveSpeed,
		Category = "NF|Monster|Attributes")
	FGameplayAttributeData MoveSpeed;
	NF_ATTRIBUTE_ACCESSORS(UNF_MonsterAttributeSet, MoveSpeed)
	
protected:
	UFUNCTION()
	void OnRep_Health(
		const FGameplayAttributeData& OldHealth);

	UFUNCTION()
	void OnRep_MaxHealth(
		const FGameplayAttributeData& OldMaxHealth);

	UFUNCTION()
	void OnRep_AttackPower(
		const FGameplayAttributeData& OldAttackPower);

	UFUNCTION()
	void OnRep_MoveSpeed(
		const FGameplayAttributeData& OldMoveSpeed);
};
