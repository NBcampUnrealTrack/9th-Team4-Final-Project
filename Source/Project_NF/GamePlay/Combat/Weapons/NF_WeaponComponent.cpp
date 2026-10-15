#include "GamePlay/Combat/Weapons/NF_WeaponComponent.h"

#include "GameFramework/Character.h"
#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GAS/Abilities/NF_GA_WeaponFire.h"
#include "Net/UnrealNetwork.h"

UNF_WeaponComponent::UNF_WeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	FireAbilityClass = UNF_GA_WeaponFire::StaticClass();
}

void UNF_WeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner()->HasAuthority() == false)
		return;
	
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
			if (EquipWeapon(Weapon) == false)
			{
				Weapon->Destroy();
			}
		}
	}
}

bool UNF_WeaponComponent::EquipWeapon(ANF_WeaponBase* NewWeapon)
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	
	if (IsValid(NewWeapon) == false || IsValid(OwnerCharacter) == false)
		return false;
	
	if (OwnerCharacter->HasAuthority() == false)
		return false;
	
	if(bChangingWeapon)
		return false;
	
	if (NewWeapon->GetWorld() != GetWorld() || IsValid(OwnerCharacter->GetMesh()) ==false ||
		IsValid(NewWeapon->GetRootComponent()) == false)
		return false;

	if (NewWeapon->GetOwner() && NewWeapon->GetOwner() != OwnerCharacter)
		return false;

	if (EquippedWeapon == NewWeapon) 
		return true;

	TGuardValue<bool> ChangeGuard(bChangingWeapon, true);
	
	if (NewWeapon->AttachToComponent(OwnerCharacter->GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocketName) == false)
		return false;

	UnequipWeaponInternal();
	NewWeapon->CancelReload();

	EquippedWeapon = NewWeapon;
	EquippedWeapon->SetOwner(OwnerCharacter);
	EquippedWeapon->SetInstigator(OwnerCharacter);
	EquippedWeapon->ForceNetUpdate();
	OwnerCharacter->ForceNetUpdate();

	return true;
}

ANF_WeaponBase* UNF_WeaponComponent::UnequipWeapon()
{
	if (IsValid(GetOwner()) == false || GetOwner()->HasAuthority() == false || bChangingWeapon)
		return nullptr;

	TGuardValue<bool> ChangeGuard(bChangingWeapon, true);
	return UnequipWeaponInternal();
}

ANF_WeaponBase* UNF_WeaponComponent::UnequipWeaponInternal()
{
	StopFire();
	
	ANF_WeaponBase* OldWeapon = nullptr;
	
	if (IsValid(EquippedWeapon))
	{
		OldWeapon = EquippedWeapon.Get();
	}
	
	EquippedWeapon = nullptr;
	
	if (OldWeapon)
	{
		OldWeapon->CancelReload();
		OldWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		OldWeapon->SetOwner(nullptr);
		OldWeapon->SetInstigator(nullptr);
		OldWeapon->ForceNetUpdate();
	}
	GetOwner()->ForceNetUpdate();
	return OldWeapon;
}

void UNF_WeaponComponent::StartFire()
{
	if (GetOwner()->HasAuthority() == false)
	{
		ServerStartFire();
		return;
	}

	if (bChangingWeapon || !IsValid(EquippedWeapon)) return;
	
	// 빠르게 재클릭해도 RPM 제한 유지
	if (bFireInputHeld) return;
	
	bFireInputHeld = true;
	
	if (EnsureFireAbility()) 
		FireASC->TryActivateAbility(FireAbilityHandle);
}

void UNF_WeaponComponent::StopFire()
{
	if (GetOwner()->HasAuthority() == false)
	{
		ServerStopFire();
		return;
	}
	
	bFireInputHeld = false;
	
	if (FireASC.IsValid() && FireAbilityHandle.IsValid())
	{
		FireASC->CancelAbilityHandle(FireAbilityHandle);
	}
}

void UNF_WeaponComponent::Reload()
{
	if (GetOwner()->HasAuthority() == false)
	{
		ServerReload();
		return;
	}

	if (bChangingWeapon) return;
	
	StopFire();
	
	if (IsValid(EquippedWeapon))
	{
		EquippedWeapon->Reload();
	}
}

bool UNF_WeaponComponent::EnsureFireAbility()
{
	if (FireASC.IsValid() && FireASC->GetAvatarActor() == GetOwner() &&
		FireASC->FindAbilitySpecFromHandle(FireAbilityHandle)) return true;
	
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	
	if (ASC == nullptr || ASC->GetAvatarActor() != GetOwner() || FireAbilityClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("WeaponFire: %s needs an initialized ASC (NF_Player / NF_PlayerState)."),
			*GetNameSafe(GetOwner()));
		return false;
	}
	
	FireASC = ASC;
	FireAbilityHandle = ASC->GiveAbility(FGameplayAbilitySpec(FireAbilityClass, 1, INDEX_NONE, this));
	return FireAbilityHandle.IsValid();
}

void UNF_WeaponComponent::ServerStartFire_Implementation() { StartFire(); }
void UNF_WeaponComponent::ServerStopFire_Implementation() { StopFire(); }
void UNF_WeaponComponent::ServerReload_Implementation() { Reload(); }

void UNF_WeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetOwner()->HasAuthority() && FireASC.IsValid())
	{
		StopFire();
		if (FireAbilityHandle.IsValid()) FireASC->ClearAbility(FireAbilityHandle);
	}
	
	Super::EndPlay(EndPlayReason);
}

void UNF_WeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UNF_WeaponComponent, EquippedWeapon);
}
