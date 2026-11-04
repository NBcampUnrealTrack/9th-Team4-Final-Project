
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "NF_PlayerStatusAttribute.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOutOfHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOutOfShield);

UCLASS()
class PROJECT_NF_API UNF_PlayerStatusAttribute : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	UNF_PlayerStatusAttribute();
	
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, MaxHealth);
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, Health);
	
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, InComingDamage)
	
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, Defence);
	
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, MaxShield);
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, Shield);
	
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	
	APlayerState* GetLethalDamageInstigatorPlayerState() const;
	
	UPROPERTY(BlueprintAssignable)
	mutable FOnOutOfHealth OnOutOfHealth;	
	
	UPROPERTY(BlueprintAssignable)
	mutable FOnOutOfShield OnOutOfShield;
	
	
protected:
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Player|Attribute")
	FGameplayAttributeData MaxHealth = 100.0f;	
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Player|Attribute")
	FGameplayAttributeData Health;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_InComingDamage, Category = "Player|Attribute")
	FGameplayAttributeData InComingDamage = 0.0f;
	
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Defence, Category = "Player|Attribute")
	FGameplayAttributeData Defence = 0.0f;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxShield, Category = "Player|Attribute")
	FGameplayAttributeData MaxShield = 100.0f;	
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Shield, Category = "Player|Attribute")
	FGameplayAttributeData Shield;
	
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);
	
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth);
	
	UFUNCTION()
	void OnRep_InComingDamage(const FGameplayAttributeData& OldInComingDamage);
	
	UFUNCTION()
	void OnRep_Defence(const FGameplayAttributeData& OldDefence);
	
	UFUNCTION()
	void OnRep_MaxShield(const FGameplayAttributeData& OldMaxShield);
	
	UFUNCTION()
	void OnRep_Shield(const FGameplayAttributeData& OldShield);


private:
	UPROPERTY(Replicated)
	bool bIsOutOfHealth = false;
	bool bIsOutOfShield = false;
	
	//킬러를 의미
	TWeakObjectPtr<APlayerState> LethalDamageInstigatorPlayerState;
	
	void BlockShieldRecovery(const FGameplayEffectModCallbackData& Data);
	
	void IsOutOfHealth(const FGameplayEffectModCallbackData& Data);
	void IsOutOfShield(const FGameplayEffectModCallbackData& Data);
};
