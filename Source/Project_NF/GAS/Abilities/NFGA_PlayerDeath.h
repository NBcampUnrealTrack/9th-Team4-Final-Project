#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "NFGA_PlayerDeath.generated.h"

UCLASS()
class PROJECT_NF_API UNFGA_PlayerDeath : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UNFGA_PlayerDeath();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
	                        bool bWasCancelled) override;
	
private:
	UFUNCTION()
	void OnFailedMontage();
	
	UFUNCTION()
	void OnCompleteMontage(FGameplayEventData EventData);
	

};
