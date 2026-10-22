#include "GAS/Abilities/Monster/NFGA_MonsterRangedAttack.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Monster/AI/NF_MonsterAIController.h"
#include "GAS/Attributes/NF_MonsterAttributeSet.h"
#include "GAS/Tags/NFTag_Monster.h"
#include "Monster/NF_Monster.h"
#include "Monster/NF_MonsterProjectile.h"

UNFGA_MonsterRangedAttack::UNFGA_MonsterRangedAttack()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

    AbilityTags.AddTag(
        NFTag_Monster::Ability_Monster_RangedAttack);

    ActivationOwnedTags.AddTag(
        NFTag_Monster::State_Monster_Attacking);

    ActivationBlockedTags.AddTag(
        NFTag_Monster::State_Monster_Dead);

    ActivationBlockedTags.AddTag(
        NFTag_Monster::State_Monster_Stunned);

    ActivationBlockedTags.AddTag(
        NFTag_Monster::State_Monster_HitReact);

    ActivationBlockedTags.AddTag(
        NFTag_Monster::State_Monster_Attacking);
}

void UNFGA_MonsterRangedAttack::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(
        Handle,
        ActorInfo,
        ActivationInfo,
        TriggerEventData);

    if (!ActorInfo || !ActorInfo->IsNetAuthority()
        || !ProjectileClass || !DamageEffect)
    {
        EndAbility(
            Handle,
            ActorInfo,
            ActivationInfo,
            true,
            true);

        return;
    }

    ANF_Monster* Monster =
        Cast<ANF_Monster>(ActorInfo->AvatarActor.Get());

    if (!Monster || !CommitAbility(
        Handle,
        ActorInfo,
        ActivationInfo))
    {
        EndAbility(
            Handle,
            ActorInfo,
            ActivationInfo,
            true,
            true);

        return;
    }

    ANF_MonsterAIController* MonsterAIController =
        Cast<ANF_MonsterAIController>(Monster->GetController());

    AActor* TargetActor = MonsterAIController
        ? MonsterAIController->GetCurrentTargetActor()
        : nullptr;

    const UNF_MonsterAttributeSet* MonsterAttributeSet =
        ActorInfo->AbilitySystemComponent->GetSet<
            UNF_MonsterAttributeSet>();

    if (!IsValid(TargetActor) || !MonsterAttributeSet)
    {
        EndAbility(
            Handle,
            ActorInfo,
            ActivationInfo,
            true,
            true);

        return;
    }

    const FVector SpawnLocation =
        Monster->GetActorLocation()
        + Monster->GetActorForwardVector()
        * ProjectileSpawnForwardOffset
        + ProjectileSpawnOffset;

    const FRotator SpawnRotation =
        (TargetActor->GetActorLocation() - SpawnLocation)
        .Rotation();

    const FTransform SpawnTransform(
        SpawnRotation,
        SpawnLocation);

    ANF_MonsterProjectile* Projectile =
        GetWorld()->SpawnActorDeferred<ANF_MonsterProjectile>(
            ProjectileClass,
            SpawnTransform,
            Monster,
            Monster,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

    if (!Projectile)
    {
        EndAbility(
            Handle,
            ActorInfo,
            ActivationInfo,
            true,
            true);

        return;
    }

    Projectile->InitializeProjectile(
        MonsterAttributeSet->GetAttackPower(),
        DamageEffect);

    Projectile->FinishSpawning(SpawnTransform);

    UAbilityTask_WaitDelay* AttackDelayTask =
        UAbilityTask_WaitDelay::WaitDelay(
            this,
            AttackInterval);

    AttackDelayTask->OnFinish.AddDynamic(
        this,
        &ThisClass::HandleAttackDelayFinished);

    AttackDelayTask->ReadyForActivation();
}

void UNFGA_MonsterRangedAttack::HandleAttackDelayFinished()
{
    EndAbility(
        CurrentSpecHandle,
        CurrentActorInfo,
        CurrentActivationInfo,
        true,
        false);
}