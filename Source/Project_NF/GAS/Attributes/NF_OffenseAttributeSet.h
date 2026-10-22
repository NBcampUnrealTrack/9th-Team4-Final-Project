#pragma once
#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "NF_OffenseAttributeSet.generated.h"

// @Owner: 이정준 (구현: Codex) | 2026-10-21
// Weapon-only bonuses. Registered by passive skills, without changing the player AttributeSet.
UCLASS()
class PROJECT_NF_API UNF_OffenseAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
public:
	UNF_OffenseAttributeSet();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackDamageBonus, Category = "NF|Offense")
	FGameplayAttributeData AttackDamageBonus;
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UNF_OffenseAttributeSet, AttackDamageBonus)
	GAMEPLAYATTRIBUTE_VALUE_GETTER(AttackDamageBonus)
	GAMEPLAYATTRIBUTE_VALUE_INITTER(AttackDamageBonus)
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_LastRoundDamageMultiplier, Category = "NF|Offense")
	FGameplayAttributeData LastRoundDamageMultiplier;
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UNF_OffenseAttributeSet, LastRoundDamageMultiplier)
	GAMEPLAYATTRIBUTE_VALUE_GETTER(LastRoundDamageMultiplier)
	GAMEPLAYATTRIBUTE_VALUE_INITTER(LastRoundDamageMultiplier)
private:
	UAbilitySystemComponent* FindRegisteredASC() const;
	void ClampValue(const FGameplayAttribute& Attribute, float& Value) const;
	UFUNCTION()
	void OnRep_AttackDamageBonus(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_LastRoundDamageMultiplier(const FGameplayAttributeData& OldValue);
};
