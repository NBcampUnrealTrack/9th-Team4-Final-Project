#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_NF_MonsterUpdateCombatSlot.generated.h"

UCLASS()
class PROJECT_NF_API UBTService_NF_MonsterUpdateCombatSlot : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_NF_MonsterUpdateCombatSlot();

protected:
	virtual void TickNode(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory,
		float DeltaSeconds) override;
};
