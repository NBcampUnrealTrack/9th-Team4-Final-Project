#include "GamePlay/Combat/Projectiles/NF_Grenade.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayEffect.h"
#include "TimerManager.h"

ANF_Grenade::ANF_Grenade()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(12.0f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Block);
	Collision->SetGenerateOverlapEvents(false);
	Collision->SetCanEverAffectNavigation(false);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCanEverAffectNavigation(false);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetUpdatedComponent(Collision);
	ProjectileMovement->InitialSpeed = 0.0f;
	ProjectileMovement->MaxSpeed = 0.0f;
	ProjectileMovement->bInitialVelocityInLocalSpace = false;
	ProjectileMovement->ProjectileGravityScale = 1.0f;
	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->Bounciness = 0.4f;
	ProjectileMovement->Friction = 0.3f;
	ProjectileMovement->bForceSubStepping = true;
}

void ANF_Grenade::InitializeGrenade(const FVector& InitialVelocity, const FGameplayEffectSpecHandle& InDamageSpec)
{
	if (HasAuthority())
	{
		DamageSpec = InDamageSpec;
		ProjectileMovement->Velocity = InitialVelocity;
	}
}

float ANF_Grenade::GetCollisionRadius() const
{
	return Collision->GetScaledSphereRadius();
}

float ANF_Grenade::GetProjectileGravityScale() const
{
	return ProjectileMovement->ProjectileGravityScale;
}

void ANF_Grenade::BeginPlay()
{
	Super::BeginPlay();
	Collision->IgnoreActorWhenMoving(GetOwner(), true);
	Collision->IgnoreActorWhenMoving(GetInstigator(), true);

	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(FuseTimer, this, &ANF_Grenade::Explode, FMath::Max(FuseTime, 0.1f), false);
	}
	else
	{
		// 클라이언트는 서버의 복제 이동을 사용한다. 별도 바운스/충돌 판정은 하지 않는다.
		ProjectileMovement->Deactivate();
		Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ANF_Grenade::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(FuseTimer);
	Super::EndPlay(EndPlayReason);
}

bool ANF_Grenade::CanAffectTarget_Implementation(AActor* Target) const
{
	return IsValid(Target) && Target != this && Target != GetOwner() && Target != GetInstigator();
}

void ANF_Grenade::Explode()
{
	if (!HasAuthority() || bExploded)
	{
		return;
	}

	bExploded = true;
	GetWorldTimerManager().ClearTimer(FuseTimer);
	ProjectileMovement->StopMovementImmediately();
	const FVector ExplosionLocation = GetActorLocation();
	ApplyExplosionDamage(ExplosionLocation);
	MulticastExplosion(ExplosionLocation);
	Destroy();
}

void ANF_Grenade::ApplyExplosionDamage(const FVector& ExplosionLocation)
{
	if (!DamageSpec.IsValid())
	{
		return;
	}

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NF_GrenadeExplosion), false, this);
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, ExplosionLocation, FQuat::Identity,
		ObjectParams, FCollisionShape::MakeSphere(FMath::Max(ExplosionRadius, 1.0f)), QueryParams);

	TSet<UAbilitySystemComponent*> AffectedComponents;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();
		if (!IsValid(Target) || !CanAffectTarget(Target))
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
		if (!TargetASC || AffectedComponents.Contains(TargetASC))
		{
			continue;
		}

		const FVector TargetLocation = Target->GetActorLocation();
		FHitResult OcclusionHit;
		if (bRequireLineOfSight && GetWorld()->LineTraceSingleByChannel(OcclusionHit,
			ExplosionLocation, TargetLocation, OcclusionTraceChannel, QueryParams)
			&& OcclusionHit.GetActor() != Target)
		{
			continue;
		}

		// 대상마다 Context를 복제하여 다른 대상의 적중 정보가 섞이지 않게 한다.
		FGameplayEffectSpec TargetSpec(*DamageSpec.Data.Get());
		FGameplayEffectContextHandle Context = TargetSpec.GetContext().Duplicate();
		Context.AddOrigin(ExplosionLocation);
		FHitResult Hit(Target, Overlap.GetComponent(), TargetLocation, (ExplosionLocation - TargetLocation).GetSafeNormal());
		Hit.TraceStart = ExplosionLocation;
		Hit.TraceEnd = TargetLocation;
		Context.AddHitResult(Hit, true);
		TargetSpec.SetContext(Context);
		AffectedComponents.Add(TargetASC);
		TargetASC->ApplyGameplayEffectSpecToSelf(TargetSpec);
	}
}

void ANF_Grenade::MulticastExplosion_Implementation(FVector_NetQuantize ExplosionLocation)
{
	if (GetNetMode() != NM_DedicatedServer)
	{
		OnExplosionCosmetic(ExplosionLocation);
	}
}
