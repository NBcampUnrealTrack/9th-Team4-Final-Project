#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_NF_MonsterBasicAttack.generated.h"

UCLASS()
class PROJECT_NF_API UBTTask_NF_MonsterBasicAttack : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_NF_MonsterBasicAttack();

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;

	virtual void TickTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory,
		float DeltaSeconds) override;

	/**
	 * Small grace distance beyond the real hit range. Once exceeded during an
	 * attack montage, the task cancels the ability so BT can resume chasing.
	 */
	UPROPERTY(EditAnywhere, Category = "NF|Monster|Combat",
		meta = (ClampMin = "0.0", Units = "cm"))
	float AttackAbortRangePadding = 75.0f;
	
};
