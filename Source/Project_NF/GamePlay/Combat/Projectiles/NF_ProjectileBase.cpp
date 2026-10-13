#include "GamePlay/Combat/Projectiles/NF_ProjectileBase.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/ProjectileMovementComponent.h"

ANF_ProjectileBase::ANF_ProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SetRootComponent(SphereComponent);

	SphereComponent->InitSphereRadius(5.f);
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	SphereComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	SphereComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	SphereComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

	SphereComponent->SetNotifyRigidBodyCollision(true);
	SphereComponent->OnComponentHit.AddDynamic(this, &ANF_ProjectileBase::OnProjectileHit);

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

void ANF_ProjectileBase::BeginPlay()
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

	if (IsValid(GetOwner()))
	{
		SphereComponent->IgnoreActorWhenMoving(GetOwner(), true);
	}

	if (IsValid(GetInstigator()))
	{
		SphereComponent->IgnoreActorWhenMoving(GetInstigator(), true);
	}
}

void ANF_ProjectileBase::InitializeProjectile(float InSpeed)
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

void ANF_ProjectileBase::IgnoreProjectile(ANF_ProjectileBase* OtherProjectile)
{
	if (IsValid(OtherProjectile) && OtherProjectile != this)
	{
		SphereComponent->IgnoreActorWhenMoving(OtherProjectile, true);
	}
}

void ANF_ProjectileBase::OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	if (HasAuthority() == false)
		return;
	
	// 투사체가 발사한 캐릭터, 총과 Hit 한 경우 무시
	if (IsValid(OtherActor) == false || OtherActor == this || OtherActor == GetOwner())
		return;
		
	const FString HitMessage = FString::Printf(
		TEXT("[Projectile Debug] %s hit %s (Component: %s)"),
		*GetNameSafe(this),
		*GetNameSafe(OtherActor),
		*GetNameSafe(OtherComponent));

	UE_LOG(LogTemp, Warning, TEXT("%s"), *HitMessage);
	
	Destroy();
}
