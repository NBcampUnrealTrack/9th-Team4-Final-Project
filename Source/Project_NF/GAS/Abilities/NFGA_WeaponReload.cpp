#include "GAS/Abilities/NFGA_WeaponReload.h"

#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"
#include "GamePlay/Combat/Weapons/NF_WeaponComponent.h"
#include "GAS/Tags/NFTag_Player.h"

UNFGA_WeaponReload::UNFGA_WeaponReload()
{
	CancelAbilitiesWithTag.AddTag(NFTag_Player::Player_State_Death);
	ActivationBlockedTags.AddTag(NFTag_Player::Player_State_Death);
	
	ActivationOwnedTags.AddTag(NFTag_Player::Player_State_Reload);

	CancelAbilitiesWithTag.AddTag(NFTag_Player::Player_State_Traversal);
	ActivationBlockedTags.AddTag(NFTag_Player::Player_State_Traversal);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
}

void UNFGA_WeaponReload::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(
		Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (ActorInfo->IsNetAuthority() == false)
	{
		PlayReloadAnimation();
		return;
	}

	WeaponComponent = Cast<UNF_WeaponComponent>(GetCurrentSourceObject());

	ANF_WeaponBase* EquippedWeapon = WeaponComponent.IsValid()
		? WeaponComponent->GetEquippedWeapon()
		: nullptr;

	if (!IsValid(EquippedWeapon)
		|| !EquippedWeapon->CanReload()
		|| !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Weapon = EquippedWeapon;

	WeaponComponent->StopFire();
	WeaponComponent->StopZoom();

	Weapon->bIsReloading = true;
	Weapon->ForceNetUpdate();

	PlayReloadAnimation();
}

void UNFGA_WeaponReload::CompleteReload()
{
	if (!IsActive() || !CurrentActorInfo || !CurrentActorInfo->IsNetAuthority()) return;
	const bool bCanComplete = WeaponComponent.IsValid() && Weapon.IsValid() &&
		WeaponComponent->GetEquippedWeapon() == Weapon.Get() && Weapon->IsReloading();
	if (bCanComplete) Weapon->FinishReload();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, !bCanComplete);
}

void UNFGA_WeaponReload::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ActorInfo && ActorInfo->IsNetAuthority() && Weapon.IsValid())
	{
		Weapon->bIsReloading = false;
		Weapon->ForceNetUpdate();
	}
	Weapon.Reset();
	WeaponComponent.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UNFGA_WeaponReload::CancelReloadAnimation()
{
	if (!IsActive() || !CurrentActorInfo || !CurrentActorInfo->IsNetAuthority()) return;

	EndAbility(CurrentSpecHandle, CurrentActorInfo,	CurrentActivationInfo, true, true);
}
