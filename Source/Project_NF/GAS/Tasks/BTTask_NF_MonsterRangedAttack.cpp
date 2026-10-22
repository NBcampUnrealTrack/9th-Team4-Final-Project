#include "GAS/Tasks/BTTask_NF_MonsterRangedAttack.h"

#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/Pawn.h"
#include "GAS/Tags/NFTag_Monster.h"

UBTTask_NF_MonsterRangedAttack::UBTTask_NF_MonsterRangedAttack()
{
	NodeName = TEXT("Monster Ranged Attack");
}

EBTNodeResult::Type UBTTask_NF_MonsterRangedAttack::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	AAIController* MonsterAIController =
		OwnerComp.GetAIOwner();

	if (!MonsterAIController)
	{
		return EBTNodeResult::Failed;
	}

	APawn* MonsterPawn =
		MonsterAIController->GetPawn();

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
		NFTag_Monster::Ability_Monster_RangedAttack);

	const bool bActivated =
		MonsterASC->TryActivateAbilitiesByTag(
			AbilityTags,
			true);

	return bActivated
		? EBTNodeResult::Succeeded
		: EBTNodeResult::Failed;
}