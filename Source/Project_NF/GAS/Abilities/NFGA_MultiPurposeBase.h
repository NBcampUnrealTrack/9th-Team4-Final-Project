#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "NFGA_MultiPurposeBase.generated.h"

UCLASS()
class PROJECT_NF_API UNFGA_MultiPurposeBase : public UGameplayAbility
{
	GENERATED_BODY()
	
protected:
	UFUNCTION()
	void OnTaskEnd();
	
	UFUNCTION()
	void OnWaitTaskEnd(FGameplayEventData EventData);
	
	UFUNCTION()
	void OnPlayerDisableMovement();

};
