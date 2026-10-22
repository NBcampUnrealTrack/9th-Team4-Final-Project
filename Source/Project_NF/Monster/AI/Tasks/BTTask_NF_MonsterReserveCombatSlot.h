#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_NF_MonsterReserveCombatSlot.generated.h"

UCLASS()
class PROJECT_NF_API UBTTask_NF_MonsterReserveCombatSlot : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_NF_MonsterReserveCombatSlot();

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, Category = "NF|Monster|Combat Slot",
		meta = (ClampMin = "0.0", Units = "cm"))
	float FallbackSlotRadius = 130.0f;

	UPROPERTY(EditAnywhere, Category = "NF|Monster|Combat Slot",
		meta = (ClampMin = "1", ClampMax = "32"))
	int32 FallbackSlotCount = 8;

	UPROPERTY(EditAnywhere, Category = "NF|Monster|Combat Slot",
		meta = (ClampMin = "0.0", Units = "cm"))
	float MinimumSlotSeparation = 100.0f;
};
