#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_NF_MonsterLookAround.generated.h"


UCLASS()
class PROJECT_NF_API UBTTask_NF_MonsterLookAround : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_NF_MonsterLookAround();

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;

	virtual void TickTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory,
		float DeltaSeconds) override;

	// 두리번 전체 시간
	UPROPERTY(
		EditAnywhere,
		Category = "NF|Monster|Look Around",
		meta = (ClampMin = "0.1", Units = "s"))
	float LookAroundDuration = 2.0f;

	//회전 최대 각도
	UPROPERTY(
		EditAnywhere,
		Category = "NF|Monster|Look Around",
		meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "deg"))
	float LookAroundAngle = 45.0f;

private:
	// Task 시작 당시 바라보던 방향
	float StartYaw = 0.0f;

	// Task가 시작된 이후 흐른 시간
	float ElapsedTime = 0.0f;	
};
