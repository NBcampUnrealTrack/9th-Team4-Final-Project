#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "NFGA_WeaponFire.generated.h"

class ANF_WeaponBase;
class UNF_WeaponComponent;

/** Shared semi-auto / automatic fire. Activated by WeaponComponent on the server. */
UCLASS()
class PROJECT_NF_API UNFGA_WeaponFire : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UNFGA_WeaponFire();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	UFUNCTION()
	void TryFire();
	void WaitForNextShot(float Delay);
	void FinishFire();
	TWeakObjectPtr<UNF_WeaponComponent> WeaponComponent;
	TWeakObjectPtr<ANF_WeaponBase> Weapon;
	bool bAutomatic = false;
};
