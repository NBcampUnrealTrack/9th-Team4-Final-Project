#include "Monster/AI/BTService_NF_MonsterUpdateCombatSlot.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/Actor.h"

namespace
{
	// Unity builds can combine this source with BT task source files into one
	// translation unit. Keep these local names distinct from task-local keys.
	const FName CombatSlotServiceTargetActorKeyName(TEXT("TargetActor"));
	const FName CombatSlotServiceLocationKeyName(TEXT("CombatSlotLocation"));
	const FName CombatSlotServiceAnchorLocationKeyName(
		TEXT("CombatSlotAnchorLocation"));
}

UBTService_NF_MonsterUpdateCombatSlot::UBTService_NF_MonsterUpdateCombatSlot()
{
	NodeName = TEXT("Update Monster Combat Slot");
	Interval = 0.15f;
	RandomDeviation = 0.0f;
}

void UBTService_NF_MonsterUpdateCombatSlot::TickNode(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BlackboardComponent =
		OwnerComp.GetBlackboardComponent();

	if (!BlackboardComponent
		|| !BlackboardComponent->IsVectorValueSet(CombatSlotServiceLocationKeyName)
		|| !BlackboardComponent->IsVectorValueSet(
			CombatSlotServiceAnchorLocationKeyName))
	{
		return;
	}

	AActor* TargetActor = Cast<AActor>(
		BlackboardComponent->GetValueAsObject(
			CombatSlotServiceTargetActorKeyName));

	if (!TargetActor)
	{
		return;
	}

	const FVector PreviousTargetLocation =
		BlackboardComponent->GetValueAsVector(
			CombatSlotServiceAnchorLocationKeyName);
	const FVector CurrentTargetLocation = TargetActor->GetActorLocation();
	const FVector TargetMovementDelta =
		CurrentTargetLocation - PreviousTargetLocation;

	if (TargetMovementDelta.IsNearlyZero())
	{
		return;
	}

	BlackboardComponent->SetValueAsVector(
		CombatSlotServiceLocationKeyName,
		BlackboardComponent->GetValueAsVector(CombatSlotServiceLocationKeyName)
			+ TargetMovementDelta);
	BlackboardComponent->SetValueAsVector(
		CombatSlotServiceAnchorLocationKeyName,
		CurrentTargetLocation);
}
