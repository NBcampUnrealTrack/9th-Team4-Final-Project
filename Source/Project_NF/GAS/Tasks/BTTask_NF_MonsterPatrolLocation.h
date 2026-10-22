#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_NF_MonsterPatrolLocation.generated.h"

class UBehaviorTreeComponent;

UCLASS()
class PROJECT_NF_API UBTTask_NF_MonsterPatrolLocation : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_NF_MonsterPatrolLocation();

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, Category = "NF|Monster|AI|Blackboard")
	FBlackboardKeySelector PatrolLocationKey;

	UPROPERTY(EditAnywhere, Category = "NF|Monster|AI",
		meta = (ClampMin = "0.0"))
	float PatrolRadius = 500.0f;	
};
