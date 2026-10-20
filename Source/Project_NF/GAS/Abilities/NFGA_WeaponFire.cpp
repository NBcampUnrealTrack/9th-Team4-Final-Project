#include "GAS/Abilities/NFGA_WeaponFire.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"
#include "GamePlay/Combat/Weapons/NF_WeaponComponent.h"
#include "GamePlay/Combat/Weapons/NF_WeaponDataAsset.h"

UNFGA_WeaponFire::UNFGA_WeaponFire()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UNFGA_WeaponFire::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	WeaponComponent = Cast<UNF_WeaponComponent>(GetCurrentSourceObject());
	Weapon = WeaponComponent.IsValid() ? WeaponComponent->GetEquippedWeapon() : nullptr;
	if (!Weapon.IsValid() || !Weapon->CanFire() || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		FinishFire();
		return;
	}
	bAutomatic = Weapon->GetWeaponData()->FireMode == ENF_FireMode::Automatic;
	TryFire();
}

void UNFGA_WeaponFire::TryFire()
{
	if (IsActive() == false)
		return;
	
	if (WeaponComponent.IsValid() == false || Weapon.IsValid() == false ||
		WeaponComponent->IsFireInputHeld() == false || WeaponComponent->GetEquippedWeapon() != Weapon.Get() ||
		Weapon->CanFire() == false)
	{
		FinishFire();
		return;
	}

	const float Remaining = Weapon->GetRemainingFireDelay();
	if (Remaining > 0.f)
	{
		WaitForNextShot(Remaining);
		return;
	}
	
	if (Weapon->FireProjectile() == false || bAutomatic == false || Weapon->CanFire() == false)
	{
		FinishFire();
		return;
	}
	
	WaitForNextShot(Weapon->GetFireInterval());
}

void UNFGA_WeaponFire::WaitForNextShot(float Delay)
{
	auto* Task = UAbilityTask_WaitDelay::WaitDelay(this, FMath::Max(Delay, 0.001f));
	Task->OnFinish.AddDynamic(this, &UNFGA_WeaponFire::TryFire);
	Task->ReadyForActivation();
}

void UNFGA_WeaponFire::FinishFire()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UNFGA_WeaponFire::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	Weapon.Reset();
	WeaponComponent.Reset();
	// Super ends owned WaitDelay tasks on release, reload, swap, or cancellation.
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
