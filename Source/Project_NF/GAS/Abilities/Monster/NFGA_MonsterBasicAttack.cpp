#include "NFGA_MonsterBasicAttack.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GAS/Tags/NFTag_Monster.h"

UNFGA_MonsterBasicAttack::UNFGA_MonsterBasicAttack()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

    AbilityTags.AddTag(
        NFTag_Monster::Ability_Monster_BasicAttack);

    ActivationOwnedTags.AddTag(
        NFTag_Monster::State_Monster_Attacking);

    ActivationBlockedTags.AddTag(
        NFTag_Monster::State_Monster_Dead);

    ActivationBlockedTags.AddTag(
        NFTag_Monster::State_Monster_Stunned);

    ActivationBlockedTags.AddTag(
        NFTag_Monster::State_Monster_Attacking);
}

void UNFGA_MonsterBasicAttack::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{     
    if (!AttackMontage || !CommitAbility(
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

    UAbilityTask_PlayMontageAndWait* MontageTask =
        UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this,
            NAME_None,
            AttackMontage);

    MontageTask->OnCompleted.AddDynamic(
        this,
        &UNFGA_MonsterBasicAttack::HandleMontageCompleted);

    MontageTask->OnInterrupted.AddDynamic(
        this,
        &UNFGA_MonsterBasicAttack::HandleMontageInterrupted);

    MontageTask->OnCancelled.AddDynamic(
        this,
        &UNFGA_MonsterBasicAttack::HandleMontageInterrupted);

    MontageTask->ReadyForActivation();
}

void UNFGA_MonsterBasicAttack::HandleMontageCompleted()
{
    EndAbility(
        CurrentSpecHandle,
        CurrentActorInfo,
        CurrentActivationInfo,
        true,
        false);
}

void UNFGA_MonsterBasicAttack::HandleMontageInterrupted()
{
    EndAbility(
        CurrentSpecHandle,
        CurrentActorInfo,
        CurrentActivationInfo,
        true,
        true);
}