#include "Monster/AI/BTService_NF_MonsterUpdateLastLc.h"

#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Monster/AI/NF_MonsterAIController.h"

UBTService_NF_MonsterUpdateLastLc::
UBTService_NF_MonsterUpdateLastLc()
{
	NodeName = TEXT("Update Monster Last Known Location");

	bNotifyTick = true;

	Interval = 0.25f;
	RandomDeviation = 0.0f;
}

void UBTService_NF_MonsterUpdateLastLc::TickNode(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	float DeltaSeconds)
{
	Super::TickNode(
		OwnerComp,
		NodeMemory,
		DeltaSeconds);

	ANF_MonsterAIController* MonsterAIController =
		Cast<ANF_MonsterAIController>(
			OwnerComp.GetAIOwner());

	if (!MonsterAIController) {	return;	}

	MonsterAIController->UpdateLastKnownLocation();
}