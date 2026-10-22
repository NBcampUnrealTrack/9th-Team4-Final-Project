#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_NF_MonsterLeapAttack.generated.h"

UCLASS()
class PROJECT_NF_API UBTTask_NF_MonsterLeapAttack : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_NF_MonsterLeapAttack();

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;
	
	virtual void TickTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	float DeltaSeconds) override;

	virtual EBTNodeResult::Type AbortTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, Category = "NF|Monster|Skill",
		meta = (ClampMin = "0.0"))
	float MinLeapDistance = 300.0f;

	UPROPERTY(EditAnywhere, Category = "NF|Monster|Skill",
		meta = (ClampMin = "0.0"))
	float MaxLeapDistance = 800.0f;
};
