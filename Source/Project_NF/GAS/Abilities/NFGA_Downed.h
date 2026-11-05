#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "NFGA_MultiPurposeBase.h"
#include "NFGA_Downed.generated.h"

UCLASS()
class PROJECT_NF_API UNFGA_Downed : public UNFGA_MultiPurposeBase
{
	GENERATED_BODY()
	
public:
	UNFGA_Downed();
	
protected:
	// 기절 상태가 시작되면 실행
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	
	// 기절 상태가 끝나면 실행
	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;
};
