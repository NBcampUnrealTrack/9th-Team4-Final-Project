#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "NFGA_WeaponReload.generated.h"

class ANF_WeaponBase;
class UNF_WeaponComponent;

/** Server-authoritative reload; cancellation never refills the magazine. */
UCLASS()
class PROJECT_NF_API UNFGA_WeaponReload : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UNFGA_WeaponReload();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	UFUNCTION()
	void CompleteReload();

	TWeakObjectPtr<UNF_WeaponComponent> WeaponComponent;
	TWeakObjectPtr<ANF_WeaponBase> Weapon;
};
