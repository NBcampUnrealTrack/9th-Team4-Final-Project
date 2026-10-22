#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_NF_MonsterUpdateLastLc.generated.h"

class UBehaviorTreeComponent;

UCLASS()
class PROJECT_NF_API UBTService_NF_MonsterUpdateLastLc : public UBTService
{
	GENERATED_BODY()
	
	public:
        UBTService_NF_MonsterUpdateLastLc();
    
    protected:
        virtual void TickNode(
            UBehaviorTreeComponent& OwnerComp,
            uint8* NodeMemory,
            float DeltaSeconds) override;
};
