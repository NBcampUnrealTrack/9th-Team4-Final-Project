#include "Monster/AI/Tasks/BTTask_NF_MonsterForgetTarget.h"

#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Monster/AI/NF_MonsterAIController.h"

UBTTask_NF_MonsterForgetTarget::UBTTask_NF_MonsterForgetTarget()
{
	NodeName = TEXT("Monster Forget Target");
}

EBTNodeResult::Type UBTTask_NF_MonsterForgetTarget::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory
)
{
	ANF_MonsterAIController* MonsterAIController =
		Cast<ANF_MonsterAIController>(OwnerComp.GetAIOwner());

	if (!MonsterAIController)
	{
		return EBTNodeResult::Failed;
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Monster investigation ended: %s"),
		*GetNameSafe(MonsterAIController->GetPawn())
	);

	MonsterAIController->ClearCurrentTargetActor();

	return EBTNodeResult::Succeeded;
}