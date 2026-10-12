#include "GamePlay/Combat/Weapons/NF_WeaponComponent.h"

#include "GameFramework/Character.h"
#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"

UNF_WeaponComponent::UNF_WeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNF_WeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(EquippedWeapon) == false && DefaultWeaponClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = GetOwner();
		Params.Instigator = Cast<APawn>(GetOwner());

		if (ANF_WeaponBase* Weapon =
			GetWorld()->SpawnActor<ANF_WeaponBase>(
				DefaultWeaponClass,
				FTransform::Identity,
				Params))
		{
			EquipWeapon(Weapon);
		}
	}
}

bool UNF_WeaponComponent::EquipWeapon(ANF_WeaponBase* NewWeapon)
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());

	if (IsValid(NewWeapon) == false || IsValid(OwnerCharacter) == false)
	{
		return false;
	}

	if (IsValid(EquippedWeapon))
	{
		EquippedWeapon->StopFire();
		EquippedWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}

	EquippedWeapon = NewWeapon;
	EquippedWeapon->SetOwner(OwnerCharacter);
	EquippedWeapon->SetInstigator(OwnerCharacter);

	return EquippedWeapon->AttachToComponent(
		OwnerCharacter->GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		WeaponSocketName);
}

void UNF_WeaponComponent::StartFire()
{
	if (IsValid(EquippedWeapon))
	{
		EquippedWeapon->StartFire();
	}
}

void UNF_WeaponComponent::StopFire()
{
	if (IsValid(EquippedWeapon))
	{
		EquippedWeapon->StopFire();
	}
}

void UNF_WeaponComponent::Reload()
{
	if (IsValid(EquippedWeapon))
	{
		EquippedWeapon->Reload();
	}
}
