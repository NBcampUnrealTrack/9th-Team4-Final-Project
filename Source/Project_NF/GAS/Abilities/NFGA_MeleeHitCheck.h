#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "NFGA_MeleeHitCheck.generated.h"


UCLASS()
class PROJECT_NF_API UNFGA_MeleeHitCheck : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UNFGA_MeleeHitCheck();


protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
							 const FGameplayAbilityActivationInfo ActivationInfo,
							 const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
							const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
							bool bWasCancelled) override;
	
	
	UFUNCTION(BlueprintCallable)
	void HitCheck(TSubclassOf<UGameplayEffect> GameplayEffectClass, float PayloadMagnitude);

	
private:
	UFUNCTION()
	void OnHitCheckValidData(const FGameplayAbilityTargetDataHandle& Data);
	
	UFUNCTION()
	void OnEndHitCheck(FGameplayEventData EventData);

	TSubclassOf<UGameplayEffect> GameplayEffectClass_CallBack;

	float PayloadMagnitude_CallBack;
};
