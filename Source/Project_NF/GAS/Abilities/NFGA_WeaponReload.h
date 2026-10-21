#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "NFGA_WeaponReload.generated.h"

class ANF_WeaponBase;
class UNF_WeaponComponent;

/** Server-authoritative reload; cancellation never refills the magazine. */
UCLASS(Blueprintable)
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
	
	// C++에서 호출하고, 자식 BP에서 구현
	UFUNCTION(BlueprintImplementableEvent, Category = "Reload")
	void PlayReloadAnimation();
	
	UFUNCTION(BlueprintCallable, Category = "Reload")
	void CompleteReload();

	UFUNCTION(BlueprintCallable, Category = "Reload")
	void CancelReloadAnimation();
	
private:
	TWeakObjectPtr<UNF_WeaponComponent> WeaponComponent;
	TWeakObjectPtr<ANF_WeaponBase> Weapon;
};
