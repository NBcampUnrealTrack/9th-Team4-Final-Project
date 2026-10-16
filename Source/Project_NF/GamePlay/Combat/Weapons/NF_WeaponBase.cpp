#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"

#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GamePlay/Combat/Weapons/NF_WeaponDataAsset.h"
#include "TimerManager.h"
#include "GamePlay/Combat/Projectiles/NF_WeaponProjectile.h"
#include "GamePlay/Combat/Weapons/NF_WeaponComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"

ANF_WeaponBase::ANF_WeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	WeaponMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMeshComponent);
	WeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ANF_WeaponBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyWeaponAppearance();
}

void ANF_WeaponBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	ApplyWeaponAppearance();
}

void ANF_WeaponBase::ApplyWeaponAppearance()
{
	if (IsValid(WeaponMeshComponent) == false || IsValid(WeaponData) == false || 
		IsValid(WeaponData->WeaponMesh) == false)
	{
		return;
	}

	WeaponMeshComponent->SetSkeletalMesh(WeaponData->WeaponMesh);
	WeaponMeshComponent->EmptyOverrideMaterials();
	const int32 SlotCount = FMath::Min(WeaponMeshComponent->GetNumMaterials(), WeaponData->WeaponMaterials.Num());
	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		if (UMaterialInterface* Material = WeaponData->WeaponMaterials[SlotIndex].Get(); IsValid(Material))
		{
			WeaponMeshComponent->SetMaterial(SlotIndex, Material);
		}
	}
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

FVector ANF_WeaponBase::GetFireDirection(const FTransform& MuzzleTransform) const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const AController* Controller = Cast<AController>(OwnerPawn->GetController());
	
	if (Controller == nullptr)
	{
		return MuzzleTransform.GetRotation().GetForwardVector();
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
	const FVector ViewDirection = ViewRotation.Vector();
	const FVector TraceEnd = ViewLocation + ViewDirection * FMath::Max(AimTraceDistance, 1.f);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WeaponAim), false);
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.AddIgnoredActor(GetInstigator());

	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit, ViewLocation, TraceEnd, ECC_Visibility, QueryParams);
	const FVector AimPoint = bHit ? Hit.ImpactPoint : TraceEnd;
	const FVector MuzzleToTarget = AimPoint - MuzzleTransform.GetLocation();

	// 가까운 장애물 때문에 탄환이 뒤로 발사되는 것을 방지
	if (FVector::DotProduct(MuzzleToTarget, ViewDirection) <= KINDA_SMALL_NUMBER)
	{
		return ViewDirection;
	}
	return MuzzleToTarget.GetSafeNormal();
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
	const FVector Forward = GetFireDirection(MuzzleTransform);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const int32 PelletCount = WeaponData->ProjectilesPerShot;
	const float SpreadRadians = FMath::DegreesToRadians(WeaponData->SpreadAngleDegrees);
	TArray<ANF_WeaponProjectile*, TInlineAllocator<64>> SpawnedProjectiles;

	for (int32 Index = 0; Index < PelletCount; ++Index)
	{
		FTransform SpawnTransform = MuzzleTransform;
		SpawnTransform.SetRotation(Forward.ToOrientationQuat());
		if (SpreadRadians > 0.f)
		{
			SpawnTransform.SetRotation(FMath::VRandCone(Forward, SpreadRadians).ToOrientationQuat());
		}

		ANF_WeaponProjectile* Projectile = GetWorld()->SpawnActor<ANF_WeaponProjectile>(
			WeaponData->ProjectileClass, SpawnTransform, SpawnParams);
		if (IsValid(Projectile) == false)
			continue;

		for (ANF_WeaponProjectile* OtherProjectile : SpawnedProjectiles)
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

void ANF_WeaponBase::CancelReload()
{
	if (HasAuthority() == false)
		return;

	GetWorldTimerManager().ClearTimer(ReloadTimerHandle);
	bIsReloading = false;
	ForceNetUpdate();
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
