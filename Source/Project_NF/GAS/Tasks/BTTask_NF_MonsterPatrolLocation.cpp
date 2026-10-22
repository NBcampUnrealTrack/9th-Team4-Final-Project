#include "GAS/Tasks/BTTask_NF_MonsterPatrolLocation.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Monster/NF_Monster.h"
#include "NavigationSystem.h"

UBTTask_NF_MonsterPatrolLocation::
UBTTask_NF_MonsterPatrolLocation()
{
	NodeName = TEXT("Find Monster Patrol Location");

	PatrolLocationKey.AddVectorFilter(
		this,
		GET_MEMBER_NAME_CHECKED(
			UBTTask_NF_MonsterPatrolLocation,
			PatrolLocationKey));
}

EBTNodeResult::Type
UBTTask_NF_MonsterPatrolLocation::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	AAIController* MonsterAIController =
		OwnerComp.GetAIOwner();

	if (!MonsterAIController)
	{
		return EBTNodeResult::Failed;
	}

	ANF_Monster* Monster =
		Cast<ANF_Monster>(MonsterAIController->GetPawn());

	if (!Monster)
	{
		return EBTNodeResult::Failed;
	}

	UBlackboardComponent* BlackboardComponent =
		OwnerComp.GetBlackboardComponent();

	if (!BlackboardComponent)
	{
		return EBTNodeResult::Failed;
	}

	UNavigationSystemV1* NavigationSystem =
		UNavigationSystemV1::GetCurrent(Monster);

	if (!NavigationSystem)
	{
		return EBTNodeResult::Failed;
	}

	FNavLocation PatrolLocation;

	const bool bFoundPatrolLocation =
		NavigationSystem->GetRandomReachablePointInRadius(
			Monster->GetHomeLocation(),
			PatrolRadius,
			PatrolLocation);

	if (!bFoundPatrolLocation)
	{
		return EBTNodeResult::Failed;
	}

	BlackboardComponent->SetValueAsVector(
		PatrolLocationKey.SelectedKeyName,
		PatrolLocation.Location);

	return EBTNodeResult::Succeeded;
}