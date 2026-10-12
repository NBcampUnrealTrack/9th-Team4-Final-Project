#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"

#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GamePlay/Combat/Weapons/NF_WeaponDataAsset.h"
#include "TimerManager.h"
#include "GamePlay/Combat/Projectiles/NF_ProjectileBase.h"

ANF_WeaponBase::ANF_WeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;

	WeaponMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMeshComponent);
	WeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ANF_WeaponBase::BeginPlay()
{
	Super::BeginPlay();
	
	if (IsValid(WeaponData))
	{
		CurrentAmmo = WeaponData->MagazineSize;
	}
}

void ANF_WeaponBase::StartFire()
{
	if (IsValid(WeaponData) == false || bIsReloading)
	{
		return;
	}

	FireOnce();
}

void ANF_WeaponBase::StopFire()
{
	GetWorldTimerManager().ClearTimer(FireTimerHandle);
}

void ANF_WeaponBase::FireOnce()
{
	if (IsValid(WeaponData) == false || bIsReloading)
	{
		return;
	}

	if (CurrentAmmo <= 0)
	{
		StopFire();
		return;
	}

	const FTransform MuzzleTransform = WeaponMeshComponent->GetSocketTransform(MuzzleSocketName,
			RTS_World);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ANF_ProjectileBase* Projectile =
		GetWorld()->SpawnActor<ANF_ProjectileBase>(
			WeaponData->ProjectileClass,
			MuzzleTransform,
			SpawnParams);

	if (IsValid(Projectile) == false)
	{
		return;
	}

	Projectile->InitializeProjectile(WeaponData->ProjectileSpeed);

	--CurrentAmmo;
}

void ANF_WeaponBase::Reload()
{
	if (!IsValid(WeaponData) ||
		bIsReloading ||
		CurrentAmmo >= WeaponData->MagazineSize)
	{
		return;
	}

	StopFire();
	bIsReloading = true;

	GetWorldTimerManager().SetTimer(
		ReloadTimerHandle,
		this,
		&ANF_WeaponBase::FinishReload,
		1.0f,
		false);
}

void ANF_WeaponBase::FinishReload()
{
	bIsReloading = false;
	CurrentAmmo = WeaponData->MagazineSize;
}

float ANF_WeaponBase::GetFireInterval() const
{
	return IsValid(WeaponData)
		? 60.f / FMath::Max(WeaponData->RoundsPerMinute, 1.f)
		: 0.2f;
}
