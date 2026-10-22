#include "Monster/AI/BTService_NF_MonsterLeash.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Monster/AI/NF_MonsterAIController.h"
#include "Monster/NF_Monster.h"

UBTService_NF_MonsterLeash::UBTService_NF_MonsterLeash()
{
	NodeName = TEXT("Monster Leash");

	bNotifyTick = true;

	Interval = 0.25f;
	RandomDeviation = 0.0f;
}

void UBTService_NF_MonsterLeash::TickNode(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	float DeltaSeconds)
{
	Super::TickNode(
		OwnerComp,
		NodeMemory,
		DeltaSeconds);

	ANF_MonsterAIController* MonsterAIController =
		Cast<ANF_MonsterAIController>(OwnerComp.GetAIOwner());

	if (!MonsterAIController) {	return;	}

	ANF_Monster* Monster =
		Cast<ANF_Monster>(MonsterAIController->GetPawn());

	if (!Monster) {	return;	}

	AActor* TargetActor =
		MonsterAIController->GetCurrentTargetActor();

	if (!IsValid(TargetActor)) { return; }

	const float DistanceSquared = FVector::DistSquared(
		Monster->GetHomeLocation(),
		TargetActor->GetActorLocation());

	if (DistanceSquared > FMath::Square(LeashRadius))
	{
		MonsterAIController->ClearCurrentTargetActor();
	}
}