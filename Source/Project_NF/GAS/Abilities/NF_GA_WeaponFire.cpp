#include "GAS/Abilities/NF_GA_WeaponFire.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"
#include "GamePlay/Combat/Weapons/NF_WeaponComponent.h"
#include "GamePlay/Combat/Weapons/NF_WeaponDataAsset.h"

UNF_GA_WeaponFire::UNF_GA_WeaponFire()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UNF_GA_WeaponFire::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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

void UNF_GA_WeaponFire::TryFire()
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

	// A rapid release/re-press must not bypass the previous shot's RPM interval.
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

void UNF_GA_WeaponFire::WaitForNextShot(float Delay)
{
	auto* Task = UAbilityTask_WaitDelay::WaitDelay(this, FMath::Max(Delay, 0.001f));
	Task->OnFinish.AddDynamic(this, &UNF_GA_WeaponFire::TryFire);
	Task->ReadyForActivation();
}

void UNF_GA_WeaponFire::FinishFire()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UNF_GA_WeaponFire::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	Weapon.Reset();
	WeaponComponent.Reset();
	// Super ends owned WaitDelay tasks on release, reload, swap, or cancellation.
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
