#include "GamePlay/Combat/Projectiles/NF_AirStrikeBomb.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GamePlay/Combat/Components/NF_AreaDamageComponent.h"
#include "UObject/ConstructorHelpers.h"

ANF_AirStrikeBomb::ANF_AirStrikeBomb()
{
	bReplicates = true;
	SetReplicateMovement(true);
	PrimaryActorTick.bCanEverTick = false;
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(10.0f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Block);
	Collision->SetGenerateOverlapEvents(false);
	Collision->SetCanEverAffectNavigation(false);
	Collision->OnComponentHit.AddDynamic(this, &ANF_AirStrikeBomb::OnBombHit);
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetRelativeScale3D(FVector(0.2f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> TestMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (TestMesh.Succeeded())
	{
		Mesh->SetStaticMesh(TestMesh.Object);
	}
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetUpdatedComponent(Collision);
	ProjectileMovement->InitialSpeed = 0.0f;
	ProjectileMovement->MaxSpeed = 0.0f;
	ProjectileMovement->bInitialVelocityInLocalSpace = false;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->bForceSubStepping = true;
	ProjectileMovement->ProjectileGravityScale = 1.0f;
	AreaDamage = CreateDefaultSubobject<UNF_AreaDamageComponent>(TEXT("AreaDamage"));
}

void ANF_AirStrikeBomb::InitializeBomb(FVector InitialVelocity, const FGameplayEffectSpecHandle& InDamageSpec)
{
	if (HasAuthority())
	{
		ProjectileMovement->Velocity = InitialVelocity;
		DamageSpec = InDamageSpec;
	}
}

void ANF_AirStrikeBomb::IgnoreOtherBomb(AActor* OtherBomb)
{
	Collision->IgnoreActorWhenMoving(OtherBomb, true);
}

float ANF_AirStrikeBomb::GetGravityScale() const
{
	return ProjectileMovement->ProjectileGravityScale;
}

void ANF_AirStrikeBomb::BeginPlay()
{
	Super::BeginPlay();
	Collision->IgnoreActorWhenMoving(GetOwner(), true);
	Collision->IgnoreActorWhenMoving(GetInstigator(), true);
	if (HasAuthority())
	{
		SetLifeSpan(FMath::Max(MaxLifetime, 1.0f));
	}
	else
	{
		ProjectileMovement->Deactivate();
		Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

bool ANF_AirStrikeBomb::CanAffectTarget_Implementation(AActor* Target) const
{
	return IsValid(Target) && Target != this && Target != GetOwner() && Target != GetInstigator();
}

bool ANF_AirStrikeBomb::FilterDamageTarget(AActor* Target)
{
	return CanAffectTarget(Target);
}

void ANF_AirStrikeBomb::OnBombHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority() || bExploded)
	{
		return;
	}
	bExploded = true;
	ProjectileMovement->StopMovementImmediately();
	const FVector Center = GetActorLocation();
	FNF_AreaDamageTargetFilter Filter;
	Filter.BindDynamic(this, &ANF_AirStrikeBomb::FilterDamageTarget);
	const float Radius = FMath::Max(ExplosionRadius, 1.0f);
	AreaDamage->SubmitSphereDamage(Center, Radius, DamageSpec, GetInstigator(), Filter);
	MulticastExplosion(Center, Radius, FMath::Max(ExplosionDebugDuration, 0.0f));
	Destroy();
}

void ANF_AirStrikeBomb::MulticastExplosion_Implementation(FVector_NetQuantize Location, float Radius, float DebugDuration)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (DebugDuration > 0.0f)
	{
		DrawDebugSphere(GetWorld(), Location, Radius, 24, FColor::Red, false, DebugDuration, 0, 2.0f);
	}
	OnExplosionCosmetic(Location);
}
