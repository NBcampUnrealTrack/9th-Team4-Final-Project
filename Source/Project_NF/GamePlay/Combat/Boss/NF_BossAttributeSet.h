#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "NF_BossAttributeSet.generated.h"

UCLASS()
class PROJECT_NF_API UNF_BossAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UNF_BossAttributeSet();

	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, Health);
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, MaxHealth);
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, IncomingDamage);
	void InitializeForBoss(float InMaxHealth);

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

protected:
	void NotifyBossHealthChanged(const FGameplayEffectModCallbackData& Data, float HealthBefore, float HealthAfter);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Boss|Attribute")
	FGameplayAttributeData Health;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Boss|Attribute")
	FGameplayAttributeData MaxHealth = 10000.0f;

	/** Server-only meta attribute, same contract as monsters/players: damage GEs add a POSITIVE amount here,
	 * PostGameplayEffectExecute subtracts it from Health and resets it to 0. Not replicated. */
	UPROPERTY(BlueprintReadOnly, Category = "Boss|Attribute")
	FGameplayAttributeData IncomingDamage;

	float HealthBeforeGameplayEffect = 0.0f;
	bool bHasHealthBeforeGameplayEffect = false;

	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);
};
