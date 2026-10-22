#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"

#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GamePlay/Combat/Weapons/NF_WeaponDataAsset.h"
#include "GamePlay/Combat/Projectiles/NF_WeaponProjectile.h"
#include "GamePlay/Combat/Weapons/NF_WeaponComponent.h"
#include "GamePlay/Combat/Damage/NF_WeaponDamageLibrary.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"

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
	
	if (OwnerPawn == nullptr)
	{
		return MuzzleTransform.GetRotation().GetForwardVector();
	}
	
	const AController* Controller = Cast<AController>(OwnerPawn->GetController());
	
	if (Controller == nullptr)
	{
		return MuzzleTransform.GetRotation().GetForwardVector();
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
	const FVector ViewDirection = ViewRotation.Vector();
	const FVector MuzzleLocation = MuzzleTransform.GetLocation();

	// 총구를 카메라 시선 위로 투영한 지점에서 트레이스 시작
	// (카메라와 캐릭터 사이의 물체에 조준점이 걸리는 것을 방지, 시선 위이므로 크로스헤어 정렬은 유지)
	const float StartOffset = FMath::Max(0.f, FVector::DotProduct(MuzzleLocation - ViewLocation, ViewDirection));
	const FVector TraceStart = ViewLocation + ViewDirection * StartOffset;
	const FVector TraceEnd = TraceStart + ViewDirection * FMath::Max(AimTraceDistance, 1.f);

	const float AimTraceRadius = IsValid(WeaponData) ? FMath::Max(WeaponData->AimTraceRadius, 0.f) : 0.f;
	constexpr float DebugDuration = 3.f;

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.AddUnique(GetOwner());
	ActorsToIgnore.AddUnique(GetInstigator());

	FHitResult Hit;
	bool bHit = UKismetSystemLibrary::SphereTraceSingle(
		this,
		TraceStart, TraceEnd,
		AimTraceRadius,
		UEngineTypes::ConvertToTraceType(ECC_Visibility),
		false,                          // bTraceComplex
		ActorsToIgnore,
		EDrawDebugTrace::ForDuration,   // 끄려면 None
		Hit,
		true,                           // bIgnoreSelf
		FLinearColor::Green,            // 충돌 전 구간
		FLinearColor::Red,              // 충돌 이후 구간
		DebugDuration);

	// 시작 지점이 이미 겹쳐 있으면 ImpactPoint를 신뢰할 수 없으므로 라인 트레이스로 대체
	if (bHit && Hit.bStartPenetrating)
	{
		bHit = UKismetSystemLibrary::LineTraceSingle(
			this, TraceStart, TraceEnd,
			UEngineTypes::ConvertToTraceType(ECC_Visibility),
			false, ActorsToIgnore, EDrawDebugTrace::ForDuration, Hit, true,
			FLinearColor::Green, FLinearColor::Red, DebugDuration);
	}

	const FVector AimPoint = bHit ? Hit.ImpactPoint : TraceEnd;
	const FVector MuzzleToTarget = AimPoint - MuzzleLocation;

	// 파랑: 카메라 위치
	DrawDebugSphere(
		GetWorld(), ViewLocation, 8.f, 12,
		FColor::Blue, false, DebugDuration);

	// 회색: 트레이스에서 건너뛴 구간 (카메라 → 트레이스 시작점)
	DrawDebugLine(
		GetWorld(), ViewLocation, TraceStart,
		FColor::Silver, false, DebugDuration, 0, 1.f);

	// 노랑: 총구 위치 (예상 발사 시작점)
	DrawDebugSphere(
		GetWorld(), MuzzleLocation, 8.f, 12,
		FColor::Yellow, false, DebugDuration);

	// 빨강: 트레이스 충돌 지점, 미충돌 시 흰색으로 끝점 표시
	DrawDebugSphere(
		GetWorld(), AimPoint, 10.f, 12,
		bHit ? FColor::Red : FColor::White, false, DebugDuration);

	// 청록: 총구에서 조준점까지의 선
	DrawDebugLine(
		GetWorld(), MuzzleLocation, AimPoint,
		FColor::Cyan, false, DebugDuration, 0, 2.f);

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
	// Snapshot before consuming ammo; all pellets from this shot use the same bonuses.
	const float ShotDamage = UNF_WeaponDamageLibrary::CalculateShotDamage(
		IsValid(GetInstigator()) ? GetInstigator() : GetOwner(), WeaponData->Damage, CurrentAmmo == 1);
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

		Projectile->InitializeProjectile(WeaponData->ProjectileSpeed, ShotDamage);
		SpawnedProjectiles.Add(Projectile);
		
		// 발사체를 Spawn한 직후, 생성된 발사체의 실제 위치 표시
		DrawDebugSphere(
			GetWorld(), Projectile->GetActorLocation(), 10.f, 12,
			FColor::Yellow, false, 3.f);
	}

	if (SpawnedProjectiles.IsEmpty())
		return false;

	--CurrentAmmo;
	LastFireTime = GetWorld()->GetTimeSeconds();
	return true;
}

void ANF_WeaponBase::Reload()
{
	if (GetOwner())
	{
		if (auto* Weapons = GetOwner()->FindComponentByClass<UNF_WeaponComponent>())
		{
			if (Weapons->GetEquippedWeapon() == this) Weapons->Reload();
		}
	}
}

bool ANF_WeaponBase::CanReload() const
{
	bool bCanReload = false;
	
	if (IsValid(WeaponData) && bIsReloading == false && CurrentAmmo < WeaponData->MagazineSize)
		bCanReload = true;
	
	return bCanReload;
}

void ANF_WeaponBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelReload();
	Super::EndPlay(EndPlayReason);
}

void ANF_WeaponBase::CancelReload()
{
	if (HasAuthority() == false)
		return;

	if (GetOwner())
	{
		if (auto* Weapons = GetOwner()->FindComponentByClass<UNF_WeaponComponent>())
		{
			if (Weapons->GetEquippedWeapon() == this) Weapons->CancelReload();
		}
	}
	bIsReloading = false;
	ForceNetUpdate();
}

void ANF_WeaponBase::FinishReload()
{
	if (!HasAuthority() || !bIsReloading) return;
	bIsReloading = false;
	
	if (IsValid(WeaponData))
	{
		CurrentAmmo = FMath::Max(0, WeaponData->MagazineSize);
	}
	ForceNetUpdate();
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
