#include "GamePlay/Combat/Projectiles/NF_WeaponProjectile.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GAS/Tags/NFTag_BossCombat.h"
#include "GamePlay/Combat/Boss/NF_BossBase.h"
#include "GamePlay/Combat/Boss/NF_BossCombatEffects.h"

ANF_WeaponProjectile::ANF_WeaponProjectile()
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

	if (IsValid(GetOwner()))
	{
		SphereComponent->IgnoreActorWhenMoving(GetOwner(), true);
	}

	if (IsValid(GetInstigator()))
	{
		SphereComponent->IgnoreActorWhenMoving(GetInstigator(), true);
	}
}

void ANF_WeaponProjectile::InitializeProjectile(float InSpeed, float InDamage)
{
	ImpactDamage = FMath::Max(0.f, InDamage);

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

	if (ANF_BossBase* Boss = Cast<ANF_BossBase>(OtherActor))
	{
		UAbilitySystemComponent* BossASC = Boss->GetAbilitySystemComponent();
		AActor* DamageSource = IsValid(GetInstigator()) ? GetInstigator() : GetOwner();
		UAbilitySystemComponent* SourceASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(DamageSource);

		if (IsValid(SourceASC) && IsValid(BossASC) && ImpactDamage > 0.f)
		{
			FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
			EffectContext.AddSourceObject(this);
			FGameplayEffectSpecHandle DamageSpec = SourceASC->MakeOutgoingSpec(
				UNFGE_PlayerDamageToBoss::StaticClass(), 1.f, EffectContext);

			if (DamageSpec.IsValid())
			{
				DamageSpec.Data->SetSetByCallerMagnitude(NFTag_BossCombat::Data_Damage, ImpactDamage);
				BossASC->ApplyGameplayEffectSpecToSelf(*DamageSpec.Data.Get());
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[WeaponProjectile] Boss hit but damage was not applied. SourceASC=%s BossASC=%s Damage=%.2f"),
				*GetNameSafe(SourceASC), *GetNameSafe(BossASC), ImpactDamage);
		}
	}
	
	Destroy();
}
