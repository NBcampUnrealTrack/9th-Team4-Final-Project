#include "GamePlay/Combat/Projectiles/NF_WeaponProjectile.h"
#include "GamePlay/Combat/Projectiles/NF_WeaponProjectileHitType.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GAS/Tags/NFGameplayCueTags.h"
#include "GamePlay/Combat/Weapons/NF_WeaponDataAsset.h"
#include "GAS/Tags/NFTag_BossCombat.h"
#include "GAS/Tags/NFTag_AttackType.h"
#include "GamePlay/Combat/Boss/NF_BossBase.h"
#include "GamePlay/Combat/Boss/NF_BossCombatEffects.h"
#include "GamePlay/Combat/Armor/NF_ArmorComponent.h"
#include "GamePlay/Combat/Armor/NF_ArmorBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GAS/Effects/NF_CommonDamageEffects.h"
#include "GamePlay/Combat/Damage/NF_DamageLibrary.h"
#include "GamePlay/Combat/Weapons/NF_WeaponComponent.h"
#include "Player/Character/NF_Player.h"
#include "Player/State/NF_PlayerState.h"
#include "GameplayEffect.h"
#include "Monster/NF_Monster.h"

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

	InitialLifeSpan = 1.f;
}

void ANF_WeaponProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() == false)
	{
		CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ProjectileMovementComponent->Deactivate();
		SetLifeSpan(0.f);
		return;
	}

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

void ANF_WeaponProjectile::InitializeProjectile(float InSpeed, float InDamage, UNF_WeaponDataAsset* InWeaponData)
{
	ShotWeaponData = InWeaponData;
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

	bHitProcessed = true;

	AActor* Attacker = IsValid(GetInstigator()) ? GetInstigator() : GetOwner();

	// 데미지/패시브 콜백으로 대상이나 투사체가 파괴되기 전에 충돌 연출을 전달한다.
	// 벽처럼 ASC가 없는 대상도 발사자의 ASC를 통해 같은 위치에 연출한다.
	if (IsValid(ShotWeaponData) && IsValid(Attacker))
	{
		if (UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Attacker))
		{
			ENF_WeaponProjectileHitType ProjectileHitType = ENF_WeaponProjectileHitType::Environment;
			if (OtherActor->IsA<ANF_BossBase>()) ProjectileHitType = ENF_WeaponProjectileHitType::Boss;
			else if (OtherActor->IsA<ANF_Monster>()) ProjectileHitType = ENF_WeaponProjectileHitType::Monster;
			else if (OtherActor->IsA<ANF_Player>()) ProjectileHitType = ENF_WeaponProjectileHitType::Player;

			FGameplayCueParameters Parameters;
			Parameters.SourceObject = ShotWeaponData;
			Parameters.Instigator = Attacker;
			Parameters.Location = Hit.ImpactPoint;
			Parameters.Normal = Hit.ImpactNormal;
			Parameters.RawMagnitude = static_cast<float>(ProjectileHitType);
			Parameters.bReplicateLocationWhenUsingMinimalRepProxy = true;
			SourceASC->ExecuteGameplayCue(NFGameplayCueTags::GameplayCue_Weapon_ProjectileHit, Parameters);
		}
	}

	const ANF_Player* HitPlayer = Cast<ANF_Player>(OtherActor);
	const APawn* AttackerPawn = Cast<APawn>(Attacker);
	const ANF_PlayerState* AttackerState = AttackerPawn ? AttackerPawn->GetPlayerState<ANF_PlayerState>() : nullptr;
	const ANF_PlayerState* HitPlayerState = HitPlayer ? HitPlayer->GetPlayerState<ANF_PlayerState>() : nullptr;
	const bool bSameTeam = IsValid(AttackerState) && IsValid(HitPlayerState)
		&& AttackerState->NF_TeamID > 0 && HitPlayerState->NF_TeamID > 0
		&& AttackerState->NF_TeamID == HitPlayerState->NF_TeamID;

	const bool bCombatTarget = HitPlayer || OtherActor->IsA<ANF_BossBase>() || OtherActor->IsA<ANF_Monster>();
	if (bCombatTarget && !bSameTeam && !OtherActor->IsActorBeingDestroyed() && IsValid(Attacker)
		&& IsValid(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Attacker))
		&& IsValid(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor)))
	{
		if (auto* Weapons = Attacker->FindComponentByClass<UNF_WeaponComponent>())
		{
			Weapons->OnAttackHit.Broadcast(Attacker, OtherActor, Hit.ImpactPoint, NFTag_AttackType::Attack_Type_Weapon);
		}
	}

	if (IsActorBeingDestroyed() || !IsValid(OtherActor) || OtherActor->IsActorBeingDestroyed())
	{
		Destroy();
		return;
	}

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

	else if (ANF_Monster* Monster = Cast<ANF_Monster>(OtherActor))
	{
		AActor* DamageSource = IsValid(GetInstigator()) ? GetInstigator() : GetOwner();
		UAbilitySystemComponent* SourceASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(DamageSource);
		UAbilitySystemComponent* MonsterASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Monster);

		if (IsValid(SourceASC) && IsValid(MonsterASC) && MonsterDamageEffectClass
			&& FMath::IsFinite(ImpactDamage) && ImpactDamage > 0.f)
		{
			FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
			EffectContext.AddSourceObject(this);
			EffectContext.AddHitResult(Hit);

			FGameplayEffectSpecHandle DamageSpec = SourceASC->MakeOutgoingSpec(
				MonsterDamageEffectClass, 1.f, EffectContext);

			if (DamageSpec.IsValid())
			{
				DamageSpec.Data->SetSetByCallerMagnitude(NFTag_BossCombat::Data_Damage, ImpactDamage);
				MonsterASC->ApplyGameplayEffectSpecToSelf(*DamageSpec.Data.Get());
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[WeaponProjectile] Monster hit but damage was not applied. SourceASC=%s MonsterASC=%s EffectClass=%s Damage=%.2f"),
				*GetNameSafe(SourceASC), *GetNameSafe(MonsterASC), *GetNameSafe(MonsterDamageEffectClass.Get()), ImpactDamage);
		}
	}

	else if (ANF_Player* TargetPlayer = Cast<ANF_Player>(OtherActor))
	{
		AActor* DamageSource = IsValid(GetInstigator()) ? GetInstigator() : GetOwner();

		if (!bSameTeam && FMath::IsFinite(ImpactDamage) && ImpactDamage > 0.f)
		{
			UAbilitySystemComponent* SourceASC =
				UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(DamageSource);
			if (IsValid(SourceASC))
			{
				FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
				EffectContext.AddSourceObject(this);
				FGameplayEffectSpecHandle DamageSpec = SourceASC->MakeOutgoingSpec(
					UNFGE_CommonDamage::StaticClass(), 1.f, EffectContext);
				if (DamageSpec.IsValid())
				{
					float AdditionalDefence = 0.f;
					const UNF_ArmorComponent* ArmorComponent = TargetPlayer->FindComponentByClass<UNF_ArmorComponent>();
					const USkeletalMeshComponent* HitMesh = Cast<USkeletalMeshComponent>(OtherComponent);
					if (ArmorComponent && IsValid(HitMesh)
						&& HitMesh == ArmorComponent->GetHitMesh() && HitMesh->GetPhysicsAsset())
					{
						const ENF_ArmorSlot Slot = ArmorComponent->ResolveHitRegion(Hit.BoneName);
						const float SlotDefence = ArmorComponent->GetAdditionalDefenseForSlot(Slot);
						if (FMath::IsFinite(SlotDefence))
						{
							AdditionalDefence = FMath::Max(0.f, SlotDefence);
						}
					}

					const float AdjustedDamage = FMath::Max(ImpactDamage - AdditionalDefence, 0.f);
					DamageSpec.Data->SetSetByCallerMagnitude(NFTag_BossCombat::Data_Damage, AdjustedDamage);
					UNF_DamageLibrary::SubmitDamageSpec(this, TargetPlayer, DamageSpec, GetActorLocation(), Hit);
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[WeaponProjectile] Player hit but source ASC is missing. Source=%s Target=%s"),
					*GetNameSafe(DamageSource), *GetNameSafe(TargetPlayer));
			}
		}
	}

	// 부위 피격 디버그 로그
	const UNF_ArmorComponent* ArmorComponent = OtherActor->FindComponentByClass<UNF_ArmorComponent>();
	const USkeletalMeshComponent* HitMesh = Cast<USkeletalMeshComponent>(OtherComponent);
	const bool bValidBodyHit = ArmorComponent && HitMesh && HitMesh == ArmorComponent->GetHitMesh() && HitMesh->GetPhysicsAsset();
	const FName BoneName = bValidBodyHit ? Hit.BoneName : NAME_None;
	const ENF_ArmorSlot Slot = ArmorComponent ? ArmorComponent->ResolveHitRegion(BoneName) : ENF_ArmorSlot::Unknown;
	const ANF_ArmorBase* Armor = ArmorComponent ? ArmorComponent->GetEquippedArmor(Slot) : nullptr;
	const float Defense = ArmorComponent ? ArmorComponent->GetAdditionalDefenseForSlot(Slot) : 0.f;
	UE_LOG(LogTemp, Warning, TEXT("[WeaponHit] Target=%s Component=%s Bone=%s Region=%s Armor=%s AdditionalDefense=%.2f"),
		*GetNameSafe(OtherActor), *GetNameSafe(OtherComponent), *BoneName.ToString(),
		*StaticEnum<ENF_ArmorSlot>()->GetNameStringByValue(static_cast<int64>(Slot)), *GetNameSafe(Armor), Defense);

	// 발사체 충돌 처리 함수 안에서, Destroy() 호출 전에 표시
	// Hit는 해당 충돌 이벤트로 받은 FHitResult
	DrawDebugSphere(
		GetWorld(), Hit.ImpactPoint, 12.f, 12,
		FColor::Magenta, false, 3.f);

	Destroy();
}
