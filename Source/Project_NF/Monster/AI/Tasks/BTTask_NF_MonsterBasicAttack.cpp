#include "Monster/AI/Tasks/BTTask_NF_MonsterBasicAttack.h"

#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/Pawn.h"
#include "GameplayAbilitySpec.h"
#include "GAS/Tags/NFTag_Monster.h"
#include "Monster/AI/NF_MonsterAIController.h"
#include "Monster/NF_Monster.h"

namespace
{
	bool IsBasicAttackAbilityActive(const UAbilitySystemComponent* MonsterASC)
	{
		if (!MonsterASC)
		{
			return false;
		}

		FGameplayTagContainer AbilityTags;
		AbilityTags.AddTag(NFTag_Monster::Ability_Monster_BasicAttack);

		TArray<FGameplayAbilitySpec*> MatchingAbilitySpecs;
		MonsterASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(
			AbilityTags,
			MatchingAbilitySpecs,
			false);

		for (const FGameplayAbilitySpec* AbilitySpec : MatchingAbilitySpecs)
		{
			if (AbilitySpec && AbilitySpec->IsActive())
			{
				return true;
			}
		}

		return false;
	}
}

UBTTask_NF_MonsterBasicAttack::UBTTask_NF_MonsterBasicAttack()
{
	NodeName = TEXT("Monster Basic Attack");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_NF_MonsterBasicAttack::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	AAIController* MonsterAIController =
		OwnerComp.GetAIOwner();

	if (!MonsterAIController) {	return EBTNodeResult::Failed; }

	APawn* MonsterPawn =
		MonsterAIController->GetPawn();

	IAbilitySystemInterface* AbilitySystemInterface =
		Cast<IAbilitySystemInterface>(MonsterPawn);

	if (!AbilitySystemInterface) { return EBTNodeResult::Failed; }

	UAbilitySystemComponent* MonsterASC =
		AbilitySystemInterface->GetAbilitySystemComponent();

	if (!MonsterASC) { return EBTNodeResult::Failed; }

	FGameplayTagContainer AbilityTags;

	AbilityTags.AddTag(
		NFTag_Monster::Ability_Monster_BasicAttack);

	const bool bActivated =
		MonsterASC->TryActivateAbilitiesByTag(
			AbilityTags,
			true);

	return bActivated
		? EBTNodeResult::InProgress
		: EBTNodeResult::Failed;
}

void UBTTask_NF_MonsterBasicAttack::TickTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* OwnerAIController = OwnerComp.GetAIOwner();
	APawn* MonsterPawn = OwnerAIController
		? OwnerAIController->GetPawn()
		: nullptr;
	IAbilitySystemInterface* AbilitySystemInterface =
		Cast<IAbilitySystemInterface>(MonsterPawn);
	UAbilitySystemComponent* MonsterASC = AbilitySystemInterface
		? AbilitySystemInterface->GetAbilitySystemComponent()
		: nullptr;

	if (!MonsterASC)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	ANF_MonsterAIController* MonsterController =
		Cast<ANF_MonsterAIController>(OwnerAIController);
	ANF_Monster* Monster = Cast<ANF_Monster>(MonsterPawn);
	AActor* TargetActor = MonsterController
		? MonsterController->GetCurrentTargetActor()
		: nullptr;

	const float AttackAbortRange = Monster
		? Monster->GetBasicAttackHitRange() + AttackAbortRangePadding
		: AttackAbortRangePadding;
	const bool bTargetOutOfAttackRange = !IsValid(TargetActor)
		|| FVector::DistSquared2D(
			MonsterPawn->GetActorLocation(),
			TargetActor->GetActorLocation()) > FMath::Square(AttackAbortRange);

	if (bTargetOutOfAttackRange)
	{
		FGameplayTagContainer AbilityTags;
		AbilityTags.AddTag(NFTag_Monster::Ability_Monster_BasicAttack);
		MonsterASC->CancelAbilities(&AbilityTags, nullptr, nullptr);

		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (!IsBasicAttackAbilityActive(MonsterASC))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
