#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "NFGA_MonsterBasicAttack.generated.h"

class UAnimMontage;

UCLASS()
class PROJECT_NF_API UNFGA_MonsterBasicAttack : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UNFGA_MonsterBasicAttack();
	
protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Monster|Attack")
	TObjectPtr<UAnimMontage> AttackMontage;

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageInterrupted();
	
};
