#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"

#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GamePlay/Combat/Weapons/NF_WeaponDataAsset.h"
#include "TimerManager.h"
#include "GamePlay/Combat/Projectiles/NF_ProjectileBase.h"
#include "GamePlay/Combat/Weapons/NF_WeaponComponent.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

ANF_WeaponBase::ANF_WeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	WeaponMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMeshComponent);
	WeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ANF_WeaponBase::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority() && IsValid(WeaponData))
	{
		CurrentAmmo = WeaponData->MagazineSize;
	}
}

void ANF_WeaponBase::StartFire()
{
	if (GetOwner())
	{
		if (auto* Weapons = GetOwner()->FindComponentByClass<UNF_WeaponComponent>())
		{
			Weapons->StartFire();
		}
	}
}

void ANF_WeaponBase::StopFire()
{
	if (GetOwner())
	{
		if (auto* Weapons = GetOwner()->FindComponentByClass<UNF_WeaponComponent>())
		{
			Weapons->StopFire();
		}
	}
}

bool ANF_WeaponBase::CanFire() const
{
	if (IsValid(WeaponData) == false || WeaponData->ProjectileClass == nullptr)
		return false;
	
	if (CurrentAmmo <= 0 || bIsReloading)
		return false;
	
	return true;
}

float ANF_WeaponBase::GetRemainingFireDelay() const
{
	float RemainingFireDelay = 0.f;
	
	if (GetWorld())
	{
		RemainingFireDelay = 
			static_cast<float>(FMath::Max(0.0, LastFireTime + GetFireInterval() - GetWorld()->GetTimeSeconds()));
	}
	
	return RemainingFireDelay;
}

bool ANF_WeaponBase::FireProjectile()
{
	if (HasAuthority() == false)
		return false;
	
	if (CanFire() == false)
		return false;
	
	if (GetRemainingFireDelay() > 0.f)
		return false;

	const FTransform MuzzleTransform = WeaponMeshComponent->GetSocketTransform(MuzzleSocketName,
			RTS_World);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const int32 PelletCount = WeaponData->ProjectilesPerShot;
	const float SpreadRadians = FMath::DegreesToRadians(WeaponData->SpreadAngleDegrees);
	const FVector Forward = MuzzleTransform.GetRotation().GetForwardVector();
	TArray<ANF_ProjectileBase*, TInlineAllocator<64>> SpawnedProjectiles;

	for (int32 Index = 0; Index < PelletCount; ++Index)
	{
		FTransform SpawnTransform = MuzzleTransform;
		if (SpreadRadians > 0.f)
		{
			SpawnTransform.SetRotation(FMath::VRandCone(Forward, SpreadRadians).ToOrientationQuat());
		}

		ANF_ProjectileBase* Projectile = GetWorld()->SpawnActor<ANF_ProjectileBase>(
			WeaponData->ProjectileClass, SpawnTransform, SpawnParams);
		if (IsValid(Projectile) == false)
			continue;

		for (ANF_ProjectileBase* OtherProjectile : SpawnedProjectiles)
		{
			Projectile->IgnoreProjectile(OtherProjectile);
			OtherProjectile->IgnoreProjectile(Projectile);
		}

		Projectile->InitializeProjectile(WeaponData->ProjectileSpeed);
		SpawnedProjectiles.Add(Projectile);
	}

	if (SpawnedProjectiles.IsEmpty())
		return false;

	--CurrentAmmo;
	LastFireTime = GetWorld()->GetTimeSeconds();
	return true;
}

void ANF_WeaponBase::Reload()
{
	if (HasAuthority() == false)
		return;
	
	if (IsValid(WeaponData) == false)
		return;

	if (bIsReloading || CurrentAmmo >= WeaponData->MagazineSize)
		return;
	
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
	
	if (IsValid(WeaponData))
	{
		CurrentAmmo = FMath::Max(0, WeaponData->MagazineSize);
	}
}

float ANF_WeaponBase::GetFireInterval() const
{
	float FireInterval = 0.2f;
	
	if (IsValid(WeaponData))
	{
		FireInterval = 60.f / FMath::Max(WeaponData->RoundsPerMinute, 1.f); 
	}
	
	return FireInterval;
}

void ANF_WeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANF_WeaponBase, CurrentAmmo);
	DOREPLIFETIME(ANF_WeaponBase, bIsReloading);
}
