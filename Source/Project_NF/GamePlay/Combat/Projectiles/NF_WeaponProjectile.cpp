#include "GamePlay/Combat/Projectiles/NF_WeaponProjectile.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GamePlay/Combat/Armor/NF_ArmorComponent.h"
#include "GamePlay/Combat/Armor/NF_ArmorBase.h"
#include "Components/SkeletalMeshComponent.h"

ANF_WeaponProjectile::ANF_WeaponProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SetRootComponent(SphereComponent);

	SphereComponent->InitSphereRadius(5.f);
	SphereComponent->SetCollisionObjectType(NF_WeaponProjectileChannel);
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	SphereComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	SphereComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	SphereComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

	SphereComponent->SetNotifyRigidBodyCollision(true);
	SphereComponent->OnComponentHit.AddDynamic(this, &ANF_WeaponProjectile::OnProjectileHit);

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	StaticMeshComponent->SetupAttachment(SphereComponent);
	StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovementComponent = 
		CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));

	ProjectileMovementComponent->UpdatedComponent = SphereComponent;
	ProjectileMovementComponent->InitialSpeed = 5000.f;
	ProjectileMovementComponent->MaxSpeed = 5000.f;
	ProjectileMovementComponent->ProjectileGravityScale = 0.f;
	ProjectileMovementComponent->bRotationFollowsVelocity = true;

	InitialLifeSpan = 2.f;
}

void ANF_WeaponProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Only the server simulates hits; clients display replicated movement.
	if (!HasAuthority())
	{
		SphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ProjectileMovementComponent->Deactivate();
		SetLifeSpan(0.f);
		return;
	}

	// Enforce the dedicated channel even on existing projectile Blueprint defaults.
	SphereComponent->SetCollisionObjectType(NF_WeaponProjectileChannel);
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComponent->SetCollisionResponseToAllChannels(ECR_Block);
	SphereComponent->SetCollisionResponseToChannel(NF_WeaponProjectileChannel, ECR_Ignore);

	if (IsValid(GetOwner()))
	{
		SphereComponent->IgnoreActorWhenMoving(GetOwner(), true);
	}

	if (IsValid(GetInstigator()))
	{
		SphereComponent->IgnoreActorWhenMoving(GetInstigator(), true);
	}
}

void ANF_WeaponProjectile::InitializeProjectile(float InSpeed)
{
	if (IsValid(ProjectileMovementComponent))
	{
		const float ProjectileSpeed = FMath::Max(0.f, InSpeed);

		ProjectileMovementComponent->InitialSpeed = ProjectileSpeed;
		ProjectileMovementComponent->MaxSpeed = ProjectileSpeed;
		ProjectileMovementComponent->Velocity =
			GetActorForwardVector() * ProjectileSpeed;
	}
}

void ANF_WeaponProjectile::IgnoreProjectile(ANF_WeaponProjectile* OtherProjectile)
{
	if (IsValid(OtherProjectile) && OtherProjectile != this)
	{
		SphereComponent->IgnoreActorWhenMoving(OtherProjectile, true);
	}
}

void ANF_WeaponProjectile::OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	if (HasAuthority() == false || bHitProcessed)
		return;
	
	// 투사체가 발사한 캐릭터, 총과 Hit 한 경우 무시
	if (IsValid(OtherActor) == false || OtherActor == this || OtherActor == GetOwner() || OtherActor == GetInstigator())
		return;
	bHitProcessed = true;
	const UNF_ArmorComponent* ArmorComponent = OtherActor->FindComponentByClass<UNF_ArmorComponent>();
	const USkeletalMeshComponent* HitMesh = Cast<USkeletalMeshComponent>(OtherComponent);
	// A capsule hit or a different visual/auxiliary mesh must never become a body-part hit.
	const bool bValidBodyHit = ArmorComponent && HitMesh && HitMesh == ArmorComponent->GetHitMesh() && HitMesh->GetPhysicsAsset();
	const FName BoneName = bValidBodyHit ? Hit.BoneName : NAME_None;
	const ENF_ArmorSlot Slot = ArmorComponent ? ArmorComponent->ResolveHitRegion(BoneName) : ENF_ArmorSlot::Unknown;
	const ANF_ArmorBase* Armor = ArmorComponent ? ArmorComponent->GetEquippedArmor(Slot) : nullptr;
	const float Defense = ArmorComponent ? ArmorComponent->GetAdditionalDefenseForSlot(Slot) : 0.f;
	UE_LOG(LogTemp, Warning, TEXT("[WeaponHit] Target=%s Component=%s Bone=%s Region=%s Armor=%s AdditionalDefense=%.2f"),
		*GetNameSafe(OtherActor), *GetNameSafe(OtherComponent), *BoneName.ToString(),
		*StaticEnum<ENF_ArmorSlot>()->GetNameStringByValue(static_cast<int64>(Slot)), *GetNameSafe(Armor), Defense);
	
	Destroy();
}
