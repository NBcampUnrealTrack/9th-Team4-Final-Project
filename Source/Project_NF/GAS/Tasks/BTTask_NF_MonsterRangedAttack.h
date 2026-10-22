#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_NF_MonsterRangedAttack.generated.h"


UCLASS()
class PROJECT_NF_API UBTTask_NF_MonsterRangedAttack : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_NF_MonsterRangedAttack();

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;	
};
