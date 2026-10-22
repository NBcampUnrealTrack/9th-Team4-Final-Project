#include "Monster/AI/Tasks/BTTask_NF_MonsterLeapAttack.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/Pawn.h"
#include "GAS/Tags/NFTag_Monster.h"
#include "Monster/AI/NF_MonsterAIController.h"

UBTTask_NF_MonsterLeapAttack::UBTTask_NF_MonsterLeapAttack()
{
	NodeName = TEXT("Monster Leap Attack");

	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_NF_MonsterLeapAttack::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	ANF_MonsterAIController* MonsterAIController =
		Cast<ANF_MonsterAIController>(OwnerComp.GetAIOwner());

	if (!MonsterAIController)
	{
		return EBTNodeResult::Failed;
	}

	APawn* MonsterPawn = MonsterAIController->GetPawn();

	if (!IsValid(MonsterPawn) || !MonsterPawn->HasAuthority())
	{
		return EBTNodeResult::Failed;
	}

	AActor* TargetActor =
		MonsterAIController->GetCurrentTargetActor();

	if (!IsValid(TargetActor))
	{
		return EBTNodeResult::Failed;
	}

	const float DistanceSquared = FVector::DistSquared2D(
		MonsterPawn->GetActorLocation(),
		TargetActor->GetActorLocation());

	if (DistanceSquared < FMath::Square(MinLeapDistance)
		|| DistanceSquared > FMath::Square(MaxLeapDistance))
	{
		return EBTNodeResult::Failed;
	}

	IAbilitySystemInterface* AbilitySystemInterface =
		Cast<IAbilitySystemInterface>(MonsterPawn);

	if (!AbilitySystemInterface)
	{
		return EBTNodeResult::Failed;
	}

	UAbilitySystemComponent* MonsterASC =
		AbilitySystemInterface->GetAbilitySystemComponent();

	if (!MonsterASC)
	{
		return EBTNodeResult::Failed;
	}

	FGameplayTagContainer AbilityTags;

	AbilityTags.AddTag(
		NFTag_Monster::Ability_Monster_LeapAttack);

	const bool bActivationRequested =
		MonsterASC->TryActivateAbilitiesByTag(
			AbilityTags,
			false);

	return bActivationRequested
		? EBTNodeResult::InProgress
		: EBTNodeResult::Failed;
}

void UBTTask_NF_MonsterLeapAttack::TickTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	float DeltaSeconds)
{
	Super::TickTask(
		OwnerComp,
		NodeMemory,
		DeltaSeconds);

	AAIController* MonsterAIController = OwnerComp.GetAIOwner();

	if (!MonsterAIController)
	{
		FinishLatentTask(
			OwnerComp,
			EBTNodeResult::Failed);

		return;
	}

	APawn* MonsterPawn = MonsterAIController->GetPawn();

	IAbilitySystemInterface* AbilitySystemInterface =
		Cast<IAbilitySystemInterface>(MonsterPawn);

	if (!AbilitySystemInterface)
	{
		FinishLatentTask(
			OwnerComp,
			EBTNodeResult::Failed);

		return;
	}

	UAbilitySystemComponent* MonsterASC =
		AbilitySystemInterface->GetAbilitySystemComponent();

	if (!MonsterASC)
	{
		FinishLatentTask(
			OwnerComp,
			EBTNodeResult::Failed);

		return;
	}

	const bool bIsAttacking =
		MonsterASC->HasMatchingGameplayTag(
			NFTag_Monster::State_Monster_Attacking);

	if (!bIsAttacking)
	{
		FinishLatentTask(
			OwnerComp,
			EBTNodeResult::Succeeded);
	}
}

// Combat분기 중단될 때 도약GA취소
EBTNodeResult::Type UBTTask_NF_MonsterLeapAttack::AbortTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	AAIController* MonsterAIController = OwnerComp.GetAIOwner();

	if (MonsterAIController)
	{
		APawn* MonsterPawn = MonsterAIController->GetPawn();

		IAbilitySystemInterface* AbilitySystemInterface =
			Cast<IAbilitySystemInterface>(MonsterPawn);

		if (AbilitySystemInterface)
		{
			UAbilitySystemComponent* MonsterASC =
				AbilitySystemInterface->GetAbilitySystemComponent();

			if (MonsterASC && MonsterPawn && MonsterPawn->HasAuthority())
			{
				FGameplayTagContainer LeapAbilityTags;

				LeapAbilityTags.AddTag(
					NFTag_Monster::Ability_Monster_LeapAttack);

				MonsterASC->CancelAbilities(
					&LeapAbilityTags);
			}
		}
	}

	return EBTNodeResult::Aborted;
}