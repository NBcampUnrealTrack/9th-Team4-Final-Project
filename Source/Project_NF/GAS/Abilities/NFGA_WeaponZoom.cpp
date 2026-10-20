#include "GAS/Abilities/NFGA_WeaponZoom.h"

#include "GamePlay/Combat/Weapons/NF_WeaponComponent.h"
#include "GAS/Tags/NFTag_Player.h"

UNFGA_WeaponZoom::UNFGA_WeaponZoom()
{
	CancelAbilitiesWithTag.AddTag(NFTag_Player::Player_State_Traversal);
	ActivationBlockedTags.AddTag(NFTag_Player::Player_State_Traversal);
	
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UNFGA_WeaponZoom::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	WeaponComponent = Cast<UNF_WeaponComponent>(GetCurrentSourceObject());
	if (!WeaponComponent.IsValid() || !WeaponComponent->CanZoom() ||
		!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	WeaponComponent->SetZoomActive(true);
}

void UNFGA_WeaponZoom::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (WeaponComponent.IsValid()) WeaponComponent->SetZoomActive(false);
	WeaponComponent.Reset();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
