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
	UPROPERTY(BlueprintReadOnly, Category = "NF|Monster|Attributes")
	FGameplayAttributeData Health;
	NF_ATTRIBUTE_ACCESSORS(UNF_MonsterAttributeSet, Health)
	
	UPROPERTY(BlueprintReadOnly, Category = "NF|Monster|Attributes")
	FGameplayAttributeData MaxHealth;
	NF_ATTRIBUTE_ACCESSORS(UNF_MonsterAttributeSet, MaxHealth)
	
	UPROPERTY(BlueprintReadOnly, Category = "NF|Monster|Attributes")
	FGameplayAttributeData AttackPower;
	NF_ATTRIBUTE_ACCESSORS(UNF_MonsterAttributeSet, AttackPower)

	UPROPERTY(BlueprintReadOnly, Category = "NF|Monster|Attributes")
	FGameplayAttributeData MoveSpeed;
	NF_ATTRIBUTE_ACCESSORS(UNF_MonsterAttributeSet, MoveSpeed)

};
