#include "GAS/Abilities/NF_GA_WeaponZoom.h"

#include "GamePlay/Combat/Weapons/NF_WeaponComponent.h"
#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"

UNF_GA_WeaponZoom::UNF_GA_WeaponZoom()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UNF_GA_WeaponZoom::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	WeaponComponent = Cast<UNF_WeaponComponent>(GetCurrentSourceObject());
	if (!WeaponComponent.IsValid() || !IsValid(WeaponComponent->GetEquippedWeapon()) ||
		!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	WeaponComponent->SetZoomActive(true);
}

void UNF_GA_WeaponZoom::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (WeaponComponent.IsValid()) WeaponComponent->SetZoomActive(false);
	WeaponComponent.Reset();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
