#include "Monster/AI/NF_MonsterAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Engine/World.h"

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
		ConsiderTargetActor(UpdatedActor);

		if (GetCurrentTargetActor() != UpdatedActor) { return; }

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
	
	AActor* ReplacementTarget =
	FindClosestPerceivedPlayer(UpdatedActor);

	if (IsValid(ReplacementTarget))
	{
		SetCurrentTargetActor(ReplacementTarget);

		BlackboardComponent->SetValueAsVector(
			LastKnownLocationKeyName,
			ReplacementTarget->GetActorLocation());

		BlackboardComponent->SetValueAsBool(
			HasLineOfSightKeyName,
			true);

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Monster target replaced: %s -> %s"),
			*GetNameSafe(UpdatedActor),
			*GetNameSafe(ReplacementTarget));

		return;
	}

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

bool ANF_MonsterAIController::IsValidPlayerTarget(
	AActor* TargetActor) const
{
	APawn* TargetPawn = Cast<APawn>(TargetActor);

	return IsValid(TargetPawn)
		&& TargetPawn->IsPlayerControlled();
}

// 타깃 교체 조건 함수
bool ANF_MonsterAIController::ShouldSwitchTarget(
	AActor* CandidateActor) const
{
	if (!IsValidPlayerTarget(CandidateActor))
	{
		return false;
	}

	AActor* CurrentTargetActor = GetCurrentTargetActor();

	// 현재 타깃이 없거나 유효하지 않다면 즉시 새 타깃 선택
	if (!IsValidPlayerTarget(CurrentTargetActor))
	{
		return true;
	}

	// 현재 타깃과 후보가 같으면 변경할 필요 x
	if (CurrentTargetActor == CandidateActor)
	{
		return false;
	}

	const UBlackboardComponent* BlackboardComponent =
		GetBlackboardComponent();

	const bool bCurrentTargetHasLineOfSight =
		BlackboardComponent
		&& BlackboardComponent->GetValueAsBool(
			HasLineOfSightKeyName);

	// 현재 타깃을 이미 놓쳤다면, 보이는 새 플레이어로 즉시 교체
	if (!bCurrentTargetHasLineOfSight)
	{
		return true;
	}

	const UWorld* World = GetWorld();

	// 타깃 잠금 시간 동안에는 새 후보가 보여도 현재 타깃 유지
	if (World
		&& World->GetTimeSeconds() < TargetLockEndTime)
	{
		return false;
	}

	APawn* MonsterPawn = GetPawn();

	if (!IsValid(MonsterPawn))
	{
		return false;
	}

	const float CurrentTargetDistance =
		FVector::Dist2D(
			MonsterPawn->GetActorLocation(),
			CurrentTargetActor->GetActorLocation());

	const float CandidateDistance =
		FVector::Dist2D(
			MonsterPawn->GetActorLocation(),
			CandidateActor->GetActorLocation());

	// 새 후보가 충분히 더 가까울 때만 타깃 바꿈
	return CandidateDistance
		+ TargetSwitchDistanceMargin
		< CurrentTargetDistance;
}

void ANF_MonsterAIController::ConsiderTargetActor(
	AActor* CandidateActor)
{
	if (!ShouldSwitchTarget(CandidateActor)) { return; }

	SetCurrentTargetActor(CandidateActor);
}

AActor* ANF_MonsterAIController::FindClosestPerceivedPlayer(
	AActor* ExcludedActor) const
{
	if (!MonsterPerceptionComponent || !SightConfig)
	{
		return nullptr;
	}

	TArray<AActor*> PerceivedActors;

	MonsterPerceptionComponent->GetCurrentlyPerceivedActors(
		SightConfig->GetSenseImplementation(),
		PerceivedActors);

	APawn* MonsterPawn = GetPawn();

	if (!IsValid(MonsterPawn))
	{
		return nullptr;
	}

	AActor* ClosestPlayer = nullptr;

	float ClosestDistanceSquared = TNumericLimits<float>::Max();

	for (AActor* PerceivedActor : PerceivedActors)
	{
		if (PerceivedActor == ExcludedActor
			|| !IsValidPlayerTarget(PerceivedActor))
		{
			continue;
		}

		const float DistanceSquared =
			FVector::DistSquared2D(
				MonsterPawn->GetActorLocation(),
				PerceivedActor->GetActorLocation());

		if (DistanceSquared < ClosestDistanceSquared)
		{
			ClosestDistanceSquared = DistanceSquared;

			ClosestPlayer = PerceivedActor;
		}
	}

	return ClosestPlayer;
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

	const bool bTargetChanged =
	GetCurrentTargetActor() != NewTargetActor;

	if (bTargetChanged)
	{
		BlackboardComponent->SetValueAsBool(
			HasLineOfSightKeyName,
			false);

		BlackboardComponent->ClearValue(
			LastKnownLocationKeyName);

		BlackboardComponent->ClearValue(
			CombatSlotLocationKeyName);

		BlackboardComponent->ClearValue(
			CombatSlotAnchorLocationKeyName);

		if (UWorld* World = GetWorld())
		{
			TargetLockEndTime =
				World->GetTimeSeconds()
				+ TargetLockDuration;
		}

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Monster target changed: %s / Lock: %.2f seconds"),
			*GetNameSafe(NewTargetActor),
			TargetLockDuration);

		BlackboardComponent->SetValueAsObject(
			TargetActorKeyName,
			NewTargetActor);

		SetFocus(NewTargetActor);
	}
}

void ANF_MonsterAIController::ClearCurrentTargetActor()
{
	UBlackboardComponent* BlackboardComponent =
		GetBlackboardComponent();

	if (!BlackboardComponent) {	return;	}

	ClearFocus(EAIFocusPriority::Gameplay);
		
	TargetLockEndTime = 0.0f;

	BlackboardComponent->SetValueAsBool(
		HasLineOfSightKeyName,
		false);

	BlackboardComponent->ClearValue(
		LastKnownLocationKeyName);

	BlackboardComponent->ClearValue(
		CombatSlotLocationKeyName);

	BlackboardComponent->ClearValue(
		CombatSlotAnchorLocationKeyName);

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
