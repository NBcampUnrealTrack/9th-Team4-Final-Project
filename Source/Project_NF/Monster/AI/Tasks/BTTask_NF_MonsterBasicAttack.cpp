#include "Monster/AI/Tasks/BTTask_NF_MonsterBasicAttack.h"

#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/Pawn.h"
#include "GAS/Tags/NFTag_Monster.h"

UBTTask_NF_MonsterBasicAttack::UBTTask_NF_MonsterBasicAttack()
{
	NodeName = TEXT("Monster Basic Attack");
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
		? EBTNodeResult::Succeeded
		: EBTNodeResult::Failed;
}