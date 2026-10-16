#include "Monster/NF_MonsterProjectile.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/SphereComponent.h"
#include "GAS/Tags/NFTag_Monster.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayEffect.h"

ANF_MonsterProjectile::ANF_MonsterProjectile()
{
    bReplicates = true;

    CollisionComponent =
        CreateDefaultSubobject<USphereComponent>(
            TEXT("CollisionComponent"));

    SetRootComponent(CollisionComponent);

    CollisionComponent->InitSphereRadius(15.0f);
    CollisionComponent->SetCollisionEnabled(
        ECollisionEnabled::QueryOnly);

    CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
    CollisionComponent->SetCollisionResponseToAllChannels(
        ECR_Ignore);

    CollisionComponent->SetCollisionResponseToChannel(
        ECC_Pawn,
        ECR_Overlap);

    CollisionComponent->SetCollisionResponseToChannel(
        ECC_WorldStatic,
        ECR_Block);

    CollisionComponent->SetGenerateOverlapEvents(true);

    CollisionComponent->OnComponentBeginOverlap.AddDynamic(
        this,
        &ThisClass::HandleProjectileOverlap);
    
    CollisionComponent->OnComponentHit.AddDynamic(
        this,
        &ThisClass::HandleProjectileHit);

    ProjectileMovementComponent =
        CreateDefaultSubobject<UProjectileMovementComponent>(
            TEXT("ProjectileMovementComponent"));

    ProjectileMovementComponent->InitialSpeed = 1200.0f;
    ProjectileMovementComponent->MaxSpeed = 1200.0f;
    ProjectileMovementComponent->bRotationFollowsVelocity = true;
    ProjectileMovementComponent->ProjectileGravityScale = 0.0f;
}

void ANF_MonsterProjectile::BeginPlay()
{
    Super::BeginPlay();

    SetLifeSpan(LifeSpan);
}

void ANF_MonsterProjectile::InitializeProjectile(
    float InDamageAmount,
    TSubclassOf<UGameplayEffect> InDamageEffect)
{
    DamageAmount = InDamageAmount;
    DamageEffect = InDamageEffect;
}

void ANF_MonsterProjectile::HandleProjectileHit(
    UPrimitiveComponent* HitComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComponent,
    FVector NormalImpulse,
    const FHitResult& Hit)
{
    if (HasAuthority())
    {
        Destroy();
    }
}

void ANF_MonsterProjectile::HandleProjectileOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComponent,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!HasAuthority()
       || !IsValid(OtherActor)
       || OtherActor == GetOwner())
    {
        return;
    }

    APawn* HitPawn = Cast<APawn>(OtherActor);

    if (!HitPawn || !HitPawn->IsPlayerControlled())
    {
        return;
    }

    ApplyDamageToPlayer(OtherActor);
}

void ANF_MonsterProjectile::ApplyDamageToPlayer(
    AActor* TargetActor)
{
    IAbilitySystemInterface* SourceAbilitySystemInterface =
        Cast<IAbilitySystemInterface>(GetOwner());

    IAbilitySystemInterface* TargetAbilitySystemInterface =
        Cast<IAbilitySystemInterface>(TargetActor);

    if (!SourceAbilitySystemInterface
        || !TargetAbilitySystemInterface
        || !DamageEffect)
    {
        Destroy();

        return;
    }

    UAbilitySystemComponent* SourceASC =
        SourceAbilitySystemInterface->GetAbilitySystemComponent();

    UAbilitySystemComponent* TargetASC =
        TargetAbilitySystemInterface->GetAbilitySystemComponent();

    if (!SourceASC || !TargetASC)
    {
        Destroy();

        return;
    }

    FGameplayEffectContextHandle EffectContext =
        SourceASC->MakeEffectContext();

    EffectContext.AddSourceObject(this);

    FGameplayEffectSpecHandle EffectSpecHandle =
        SourceASC->MakeOutgoingSpec(
            DamageEffect,
            1.0f,
            EffectContext);

    if (!EffectSpecHandle.IsValid())
    {
        Destroy();

        return;
    }

    EffectSpecHandle.Data->SetSetByCallerMagnitude(
        NFTag_Monster::Data_Monster_Damage,
        -DamageAmount);

    SourceASC->ApplyGameplayEffectSpecToTarget(
        *EffectSpecHandle.Data.Get(),
        TargetASC);

    Destroy();
}