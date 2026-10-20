#include "GamePlay/Combat/Projectiles/NF_WeaponProjectile.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GAS/Tags/NFTag_BossCombat.h"
#include "GamePlay/Combat/Boss/NF_BossBase.h"
#include "GamePlay/Combat/Boss/NF_BossCombatEffects.h"
#include "GamePlay/Combat/Armor/NF_ArmorComponent.h"
#include "GamePlay/Combat/Armor/NF_ArmorBase.h"
#include "Components/SkeletalMeshComponent.h"


ANF_WeaponProjectile::ANF_WeaponProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	SetRootComponent(CapsuleComponent);

	CapsuleComponent->InitCapsuleSize(5.f, 10.f);
	CapsuleComponent->SetCollisionObjectType(NF_WeaponProjectileChannel);
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	CapsuleComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CapsuleComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CapsuleComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CapsuleComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

	CapsuleComponent->SetNotifyRigidBodyCollision(true);
	CapsuleComponent->OnComponentHit.AddDynamic(this, &ANF_WeaponProjectile::OnProjectileHit);

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	StaticMeshComponent->SetupAttachment(CapsuleComponent);
	StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovementComponent = 
		CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));

	ProjectileMovementComponent->UpdatedComponent = CapsuleComponent;
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
		CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ProjectileMovementComponent->Deactivate();
		SetLifeSpan(0.f);
		return;
	}

	// Enforce the dedicated channel even on existing projectile Blueprint defaults.
	CapsuleComponent->SetCollisionObjectType(NF_WeaponProjectileChannel);
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CapsuleComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CapsuleComponent->SetCollisionResponseToChannel(NF_WeaponProjectileChannel, ECR_Ignore);

	if (IsValid(GetOwner()))
	{
		CapsuleComponent->IgnoreActorWhenMoving(GetOwner(), true);
	}

	if (IsValid(GetInstigator()))
	{
		CapsuleComponent->IgnoreActorWhenMoving(GetInstigator(), true);
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
		CapsuleComponent->IgnoreActorWhenMoving(OtherProjectile, true);
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
