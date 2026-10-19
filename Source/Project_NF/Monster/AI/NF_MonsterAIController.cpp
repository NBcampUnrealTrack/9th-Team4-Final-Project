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
	
	SetPerceptionComponent(*MonsterPerceptionComponent);

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
	
	MonsterPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(
	this,
	&ANF_MonsterAIController::HandleTargetPerceptionUpdated);
}

void ANF_MonsterAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!MonsterBehaviorTree)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Monster AI: Behavior Tree가 지정되지 않았습니다."));

		return;
	}

	const bool bBehaviorTreeStarted =
		RunBehaviorTree(MonsterBehaviorTree);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Monster AI: Behavior Tree 시작 결과 = %s"),
		bBehaviorTreeStarted ? TEXT("True") : TEXT("False"));
}

void ANF_MonsterAIController::HandleTargetPerceptionUpdated(
	AActor* UpdatedActor,
	FAIStimulus Stimulus)
{
	APawn* DetectedPawn = Cast<APawn>(UpdatedActor);

	if (!DetectedPawn || !DetectedPawn->IsPlayerControlled()) {	return;	}

	UBlackboardComponent* BlackboardComponent =
		GetBlackboardComponent();

	if (!BlackboardComponent)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Monster AI: BlackboardComponent is null."));

		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		SetCurrentTargetActor(UpdatedActor);

		BlackboardComponent->SetValueAsVector(
			LastKnownLocationKeyName,
			UpdatedActor->GetActorLocation());

		BlackboardComponent->SetValueAsBool(
			HasLineOfSightKeyName,
			true);

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Monster sight gained: %s / LastKnownLocation: %s"),
			*GetNameSafe(UpdatedActor),
			*BlackboardComponent->GetValueAsVector(
				LastKnownLocationKeyName).ToString());

		return;
	}
	
	if (GetCurrentTargetActor() != UpdatedActor) { return; }

	BlackboardComponent->SetValueAsBool(
		HasLineOfSightKeyName,
		false);

	ClearFocus(EAIFocusPriority::Gameplay);

	UE_LOG(
	LogTemp,
	Warning,
	TEXT("Monster sight lost: %s / LastKnownLocation: %s"),
	*GetNameSafe(UpdatedActor),
	*BlackboardComponent->GetValueAsVector(
		LastKnownLocationKeyName).ToString());
}

AActor* ANF_MonsterAIController::GetCurrentTargetActor() const
{
	const UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();

	if (!BlackboardComponent)
	{
		return nullptr;
	}

	return Cast<AActor>(
		BlackboardComponent->GetValueAsObject(TargetActorKeyName));
}

void ANF_MonsterAIController::SetCurrentTargetActor(
	AActor* NewTargetActor)
{
	APawn* TargetPawn = Cast<APawn>(NewTargetActor);

	if (!TargetPawn || !TargetPawn->IsPlayerControlled())
	{
		return;
	}

	UBlackboardComponent* BlackboardComponent =
		GetBlackboardComponent();

	if (!BlackboardComponent) {	return;	}

	if (GetCurrentTargetActor() != NewTargetActor)
	{
		BlackboardComponent->SetValueAsBool(
			HasLineOfSightKeyName,
			false);

		BlackboardComponent->ClearValue(
			LastKnownLocationKeyName);
	}

	BlackboardComponent->SetValueAsObject(
		TargetActorKeyName,
		NewTargetActor);

	SetFocus(NewTargetActor);
}

void ANF_MonsterAIController::ClearCurrentTargetActor()
{
	UBlackboardComponent* BlackboardComponent =
		GetBlackboardComponent();

	if (!BlackboardComponent) {	return;	}

	ClearFocus(EAIFocusPriority::Gameplay);

	BlackboardComponent->SetValueAsBool(
		HasLineOfSightKeyName,
		false);

	BlackboardComponent->ClearValue(
		LastKnownLocationKeyName);

	BlackboardComponent->ClearValue(
		TargetActorKeyName);
}

void ANF_MonsterAIController::UpdateLastKnownLocation()
{
	UBlackboardComponent* BlackboardComponent =
		GetBlackboardComponent();

	if (!BlackboardComponent) {	return;	}

	const bool bHasLineOfSight =
		BlackboardComponent->GetValueAsBool(
			HasLineOfSightKeyName);

	if (!bHasLineOfSight) {	return;	}

	AActor* TargetActor = GetCurrentTargetActor();

	if (!IsValid(TargetActor)) { return; }

	BlackboardComponent->SetValueAsVector(
		LastKnownLocationKeyName,
		TargetActor->GetActorLocation());
}