#include "Monster/AI/NF_MonsterAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

ANF_MonsterAIController::ANF_MonsterAIController()
{
	MonsterPerceptionComponent =
		CreateDefaultSubobject<UAIPerceptionComponent>(
			TEXT("MonsterPerceptionComponent"));

	SightConfig =
		CreateDefaultSubobject<UAISenseConfig_Sight>(
			TEXT("SightConfig"));

	SightConfig->SightRadius = 1200.0f;
	SightConfig->LoseSightRadius = 1500.0f;
	SightConfig->PeripheralVisionAngleDegrees = 70.0f;
	SightConfig->SetMaxAge(3.0f);

	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

	MonsterPerceptionComponent->ConfigureSense(*SightConfig);

	MonsterPerceptionComponent->SetDominantSense(
		SightConfig->GetSenseImplementation());
}

void ANF_MonsterAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (MonsterBehaviorTree)
	{
		RunBehaviorTree(MonsterBehaviorTree);
	}
}

void ANF_MonsterAIController::HandleTargetPerceptionUpdated(
	AActor* UpdatedActor,
	FAIStimulus Stimulus)
{
	APawn* DetectedPawn = Cast<APawn>(UpdatedActor);

	if (!DetectedPawn || !DetectedPawn->IsPlayerControlled()) {	return;	}

	UBlackboardComponent* BlackboardComponent =
		GetBlackboardComponent();

	if (!BlackboardComponent) {	return;	}

	if (Stimulus.WasSuccessfullySensed())
	{
		BlackboardComponent->SetValueAsObject(
			TargetActorKeyName,
			UpdatedActor);

		return;
	}

	if (BlackboardComponent->GetValueAsObject(TargetActorKeyName)
		== UpdatedActor)
	{
		BlackboardComponent->ClearValue(TargetActorKeyName);
	}
}