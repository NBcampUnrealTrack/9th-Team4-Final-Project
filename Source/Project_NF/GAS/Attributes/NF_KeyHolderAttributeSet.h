#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "NF_KeyHolderAttributeSet.generated.h"

#define NF_KEYHOLDER_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName) 

UCLASS()
class PROJECT_NF_API UNF_KeyHolderAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	UNF_KeyHolderAttributeSet();
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	// 열쇠 보유시 쿨감 30%
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CooldownReductionRate, Category = "KeyHolder")
	FGameplayAttributeData CooldownReductionRate;
	
	NF_KEYHOLDER_ATTRIBUTE_ACCESSORS(UNF_KeyHolderAttributeSet, CooldownReductionRate)
	
	// 열쇠 보유시 받는 피해 감소량 30%
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DamageReductionRate, Category = "KeyHolder")
	FGameplayAttributeData DamageReductionRate;
	
	NF_KEYHOLDER_ATTRIBUTE_ACCESSORS(UNF_KeyHolderAttributeSet, DamageReductionRate)
	
protected:
	UFUNCTION()
	void OnRep_CooldownReductionRate(const FGameplayAttributeData& OldValue);
	
	UFUNCTION()
	void OnRep_DamageReductionRate(const FGameplayAttributeData& OldValue);
};
