#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_NF_MonsterLeash.generated.h"

class UBehaviorTreeComponent;

UCLASS()
class PROJECT_NF_API UBTService_NF_MonsterLeash : public UBTService
{
	GENERATED_BODY()
	
public:
	UBTService_NF_MonsterLeash();

protected:
	virtual void TickNode(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory,
		float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "NF|Monster|AI",
		meta = (ClampMin = "0.0"))
	float LeashRadius = 1800.0f;
};
