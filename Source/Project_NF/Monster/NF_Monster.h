#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "NF_Monster.generated.h"

class UNF_AbilitySystemComponent;
class UNF_MonsterAttributeSet;
class UGameplayEffect;
class UGameplayAbility;
class UAnimMontage;

UCLASS()
class PROJECT_NF_API ANF_Monster : public ACharacter , public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ANF_Monster();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	void ApplyBasicAttackHit();
	
protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Monster|GAS")
	TObjectPtr<UNF_AbilitySystemComponent> MonsterASC;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Monster|GAS")
	TObjectPtr<UNF_MonsterAttributeSet> MonsterAttributeSet;	
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Monster|GAS")
	TSubclassOf<UGameplayEffect> DefaultAttributesEffect;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Monster|GAS")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;
	
	UPROPERTY(EditDefaultsOnly, Category = "NF|Monster|Combat")
	TSubclassOf<UGameplayEffect> BasicAttackDamageEffect;

	UPROPERTY(EditDefaultsOnly, Category = "NF|Monster|Combat",
		meta = (ClampMin = "0.0"))
	float BasicAttackHitRange = 250.0f;
	
	//hit
	UFUNCTION()
	void HandleMonsterDamaged(float Damage);

	UFUNCTION()
	void ClearHitReactState();

	UPROPERTY(EditDefaultsOnly, Category = "NF|Monster|Combat")
	TObjectPtr<UAnimMontage> HitReactMontage;

	FTimerHandle HitReactTimerHandle;
	
	//death
	UFUNCTION()
	void HandleMonsterOutOfHealth();

	UPROPERTY(EditDefaultsOnly, Category = "NF|Monster|Combat")
	TObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY(EditDefaultsOnly, Category = "NF|Monster|Combat",
		meta = (ClampMin = "0.0"))
	float CorpseLifeSpan = 5.0f;

	bool bIsDead = false;
};
