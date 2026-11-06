#pragma once

#include "CoreMinimal.h"
#include "NFGA_MultiPurposeBase.h"
#include "NFGA_HitReact.generated.h"

UCLASS()
class PROJECT_NF_API UNFGA_HitReact : public UNFGA_MultiPurposeBase
{
	GENERATED_BODY()

public:
	UNFGA_HitReact();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;
	
private:
	bool bIsBeenAlreadyAction = false;
	
	UPROPERTY()
	TObjectPtr<UAnimMontage> HitReactMontage;
};
