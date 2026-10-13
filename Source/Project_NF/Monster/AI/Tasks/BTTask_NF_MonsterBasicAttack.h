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
	
};
