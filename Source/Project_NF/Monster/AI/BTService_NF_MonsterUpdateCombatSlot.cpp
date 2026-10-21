#include "Monster/AI/BTService_NF_MonsterUpdateCombatSlot.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/Actor.h"

namespace
{
	const FName TargetActorKeyName(TEXT("TargetActor"));
	const FName CombatSlotLocationKeyName(TEXT("CombatSlotLocation"));
	const FName CombatSlotAnchorLocationKeyName(TEXT("CombatSlotAnchorLocation"));
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
		|| !BlackboardComponent->IsVectorValueSet(CombatSlotLocationKeyName)
		|| !BlackboardComponent->IsVectorValueSet(
			CombatSlotAnchorLocationKeyName))
	{
		return;
	}

	AActor* TargetActor = Cast<AActor>(
		BlackboardComponent->GetValueAsObject(TargetActorKeyName));

	if (!TargetActor)
	{
		return;
	}

	const FVector PreviousTargetLocation =
		BlackboardComponent->GetValueAsVector(
			CombatSlotAnchorLocationKeyName);
	const FVector CurrentTargetLocation = TargetActor->GetActorLocation();
	const FVector TargetMovementDelta =
		CurrentTargetLocation - PreviousTargetLocation;

	if (TargetMovementDelta.IsNearlyZero())
	{
		return;
	}

	BlackboardComponent->SetValueAsVector(
		CombatSlotLocationKeyName,
		BlackboardComponent->GetValueAsVector(CombatSlotLocationKeyName)
			+ TargetMovementDelta);
	BlackboardComponent->SetValueAsVector(
		CombatSlotAnchorLocationKeyName,
		CurrentTargetLocation);
}
