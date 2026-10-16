#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "NF_GA_WeaponZoom.generated.h"

class UNF_WeaponComponent;

UCLASS()
class PROJECT_NF_API UNF_GA_WeaponZoom : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UNF_GA_WeaponZoom();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	TWeakObjectPtr<UNF_WeaponComponent> WeaponComponent;
};
