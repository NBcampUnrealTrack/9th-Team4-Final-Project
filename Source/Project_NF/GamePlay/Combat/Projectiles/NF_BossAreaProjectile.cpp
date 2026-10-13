#include "GamePlay/Combat/Projectiles/NF_BossAreaProjectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

ANF_BossAreaProjectile::ANF_BossAreaProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(8.0f);
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Collision->SetGenerateOverlapEvents(false);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetUpdatedComponent(Collision);
	ProjectileMovement->InitialSpeed = 0.0f;
	ProjectileMovement->MaxSpeed = 0.0f;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->bRotationFollowsVelocity = true;
}

void ANF_BossAreaProjectile::InitializeProjectile(const FVector& InDestination, float InDamageRadius,
	float InSpeed, const FGameplayEffectSpecHandle& InDamageSpec)
{
	if (!HasAuthority())
	{
		return;
	}
	Destination = InDestination;
	DamageRadius = FMath::Max(InDamageRadius, 1.0f);
	DamageSpec = InDamageSpec;
	const FVector Direction = (FVector(Destination) - GetActorLocation()).GetSafeNormal();
	ProjectileMovement->Velocity = Direction * FMath::Max(InSpeed, 1.0f);
}

void ANF_BossAreaProjectile::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority())
	{
		ProjectileMovement->Deactivate();
		return;
	}

	const float TravelTime = FVector::Distance(GetActorLocation(), FVector(Destination)) /
		FMath::Max(ProjectileMovement->Velocity.Size(), 1.0f);
	GetWorldTimerManager().SetTimer(ArrivalTimer, this, &ANF_BossAreaProjectile::ReachDestination,
		FMath::Max(TravelTime, 0.01f), false);
}

void ANF_BossAreaProjectile::ReachDestination()
{
	if (!HasAuthority())
	{
		return;
	}
	SetActorLocation(Destination, false, nullptr, ETeleportType::TeleportPhysics);
	ProjectileMovement->StopMovementImmediately();
	ApplyImpactDamage();
	Destroy();
}

void ANF_BossAreaProjectile::ApplyImpactDamage()
{
	if (!DamageSpec.IsValid() || !GetWorld())
	{
		return;
	}

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NF_BossAreaImpact), false, this);
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, FVector(Destination), FQuat::Identity,
		ObjectParams, FCollisionShape::MakeSphere(DamageRadius), QueryParams);

	TSet<UAbilitySystemComponent*> AffectedComponents;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
		if (!IsValid(Target) || !TargetASC || AffectedComponents.Contains(TargetASC))
		{
			continue;
		}
		FGameplayEffectSpec TargetSpec(*DamageSpec.Data.Get());
		FGameplayEffectContextHandle Context = TargetSpec.GetContext().Duplicate();
		Context.AddOrigin(Destination);
		TargetSpec.SetContext(Context);
		AffectedComponents.Add(TargetASC);
		TargetASC->ApplyGameplayEffectSpecToSelf(TargetSpec);
	}
}

void ANF_BossAreaProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANF_BossAreaProjectile, Destination);
	DOREPLIFETIME(ANF_BossAreaProjectile, DamageRadius);
}
