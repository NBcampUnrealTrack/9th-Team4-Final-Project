#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "NFGA_WeaponFire.generated.h"

class ANF_WeaponBase;
class UNF_WeaponComponent;

UCLASS(Blueprintable)
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

	// C++에서 호출하고, 자식 BP에서 구현, Stop When Ability Ends 체크 해제.	
	UFUNCTION(BlueprintImplementableEvent, Category = "Fire")
	void PlayFireAnimation();

private:
	UFUNCTION(Client, Reliable)
	void ClientPlayFireAnimation();

	UFUNCTION()
	void TryFire();
	void WaitForNextShot(float Delay);
	void FinishFire();
	TWeakObjectPtr<UNF_WeaponComponent> WeaponComponent;
	TWeakObjectPtr<ANF_WeaponBase> Weapon;
	bool bAutomatic = false;
};
