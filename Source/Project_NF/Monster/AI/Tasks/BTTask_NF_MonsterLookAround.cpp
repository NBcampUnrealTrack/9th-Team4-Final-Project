#include "Monster/AI/Tasks/BTTask_NF_MonsterLookAround.h"


#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"

UBTTask_NF_MonsterLookAround::UBTTask_NF_MonsterLookAround()
{
	NodeName = TEXT("Monster Look Around");
	
	bNotifyTick = true;

	bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_NF_MonsterLookAround::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();

	if (!IsValid(AIController))
	{
		return EBTNodeResult::Failed;
	}

	APawn* MonsterPawn = AIController->GetPawn();

	if (!IsValid(MonsterPawn) || !MonsterPawn->HasAuthority())
	{
		return EBTNodeResult::Failed;
	}

	// 이전 이동이 남아 있다면 정지
	AIController->StopMovement();

	// 탐색 시작 당시 몬스터가 바라보는 방향을 저장
	StartYaw = MonsterPawn->GetActorRotation().Yaw;
	ElapsedTime = 0.0f;

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Monster look around started: %s"),
		*GetNameSafe(MonsterPawn));

	// 즉시 끝내지 않고 TickTask에서 탐색을 계속함
	return EBTNodeResult::InProgress;
}

void UBTTask_NF_MonsterLookAround::TickTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();

	if (!IsValid(AIController))
	{
		FinishLatentTask(
			OwnerComp,
			EBTNodeResult::Failed);

		return;
	}

	APawn* MonsterPawn = AIController->GetPawn();

	if (!IsValid(MonsterPawn) || !MonsterPawn->HasAuthority())
	{
		FinishLatentTask(
			OwnerComp,
			EBTNodeResult::Failed);

		return;
	}

	// 플레이어를 발견했다면 전투 분기로 전환
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();

	if (Blackboard && Blackboard->GetValueAsBool(TEXT("HasLineOfSight")))
	{
		FinishLatentTask(
			OwnerComp,
			EBTNodeResult::Failed);

		return;
	}

	const float SafeDuration = FMath::Max(LookAroundDuration, 0.1f);

	ElapsedTime += DeltaSeconds;
	
	const float Progress = FMath::Clamp(
		ElapsedTime / SafeDuration,
		0.0f,
		1.0f);
	
	const float YawOffset =
		-FMath::Sin(Progress * 2.0f * PI) * LookAroundAngle;

	const FRotator CurrentRotation = MonsterPawn->GetActorRotation();

	const FRotator NewRotation(
		CurrentRotation.Pitch,
		StartYaw + YawOffset,
		CurrentRotation.Roll);

	MonsterPawn->SetActorRotation(NewRotation);

	if (Progress >= 1.0f)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("Monster look around finished: %s"),
			*GetNameSafe(MonsterPawn));

		FinishLatentTask(
			OwnerComp,
			EBTNodeResult::Succeeded);
	}
}