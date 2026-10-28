#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "TimerManager.h"
#include "NFGA_Revive.generated.h"

class APawn;

UCLASS()
class PROJECT_NF_API UNFGA_Revive : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UNFGA_Revive();
	
	bool SetReviveTarget(APawn* InTargetPawn);
	
	void ClearPendingReviveTarget();
	
protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	
	virtual void EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled) override;
	
private:
	TWeakObjectPtr<APawn> PendingReviveTarget;
	TWeakObjectPtr<APawn> ActiveReviveTarget;
	
	FTimerHandle ReviveTimerHandle;
	FTimerHandle ReviveValidationTimerHandle;
	
	// 소생 진행 시간 임시값 (5초)
	UPROPERTY(EditAnywhere, Category = "Life|Revive", meta = (ClampMin = "0.1"))
	float ReviveDurationSeconds = 5.0f;
	
	void CompleteRevive();
	void CheckReviveProgress();
};
