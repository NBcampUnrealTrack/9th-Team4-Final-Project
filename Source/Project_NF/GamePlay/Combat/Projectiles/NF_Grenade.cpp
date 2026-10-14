#include "GamePlay/Combat/Projectiles/NF_Grenade.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GamePlay/Combat/Components/NF_AreaDamageComponent.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"

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
	AreaDamage = CreateDefaultSubobject<UNF_AreaDamageComponent>(TEXT("AreaDamage"));
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

bool ANF_Grenade::FilterAreaDamageTarget(AActor* Target)
{
	return CanAffectTarget(Target);
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
	// 기존 BP의 가림 설정과 대상 필터를 유지하면서 공통 범위 판정을 사용한다.
	AreaDamage->SetOcclusionPolicy(bRequireLineOfSight, OcclusionTraceChannel);
	FNF_AreaDamageTargetFilter TargetFilter;
	TargetFilter.BindDynamic(this, &ANF_Grenade::FilterAreaDamageTarget);
	AreaDamage->SubmitSphereDamage(ExplosionLocation, GetExplosionRadius(), DamageSpec, GetInstigator(), TargetFilter);
	// 로컬 기본값 대신 실제 서버 반경을 전달한다. 0초는 표시 비활성화다.
	MulticastExplosion(ExplosionLocation, GetExplosionRadius(),
		bShowExplosionRadius ? FMath::Max(ExplosionRadiusDisplayTime, 0.1f) : 0.0f);
	Destroy();
}

void ANF_Grenade::MulticastExplosion_Implementation(FVector_NetQuantize ExplosionLocation,
	float Radius, float RadiusDisplayTime)
{
	if (GetNetMode() != NM_DedicatedServer)
	{
		if (RadiusDisplayTime > 0.0f)
		{
			// 디버그 도형의 수명은 수류탄 액터와 독립적이므로 Destroy 이후에도 잠시 남는다.
			DrawDebugSphere(GetWorld(), ExplosionLocation, Radius, 32, FColor::Red,
				false, RadiusDisplayTime, 0, 2.0f);
		}
		OnExplosionCosmetic(ExplosionLocation);
	}
}
