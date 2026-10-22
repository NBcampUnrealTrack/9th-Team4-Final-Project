#include "Monster/AI/Tasks/BTTask_NF_MonsterReserveCombatSlot.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/Pawn.h"
#include "NavigationSystem.h"

namespace
{
	struct FNF_MonsterCombatSlotReservation
	{
		TWeakObjectPtr<AActor> TargetActor;
		FVector Location = FVector::ZeroVector;
	};

	TMap<TWeakObjectPtr<APawn>, FNF_MonsterCombatSlotReservation>
		MonsterCombatSlotReservations;

	const FName TargetActorKeyName(TEXT("TargetActor"));
	const FName CombatSlotLocationKeyName(TEXT("CombatSlotLocation"));
	const FName CombatSlotAnchorLocationKeyName(TEXT("CombatSlotAnchorLocation"));

	bool IsCombatSlotReserved(
		const APawn* RequestingMonster,
		const AActor* TargetActor,
		const FVector& CandidateLocation,
		float MinimumSlotSeparation)
	{
		for (const TPair<TWeakObjectPtr<APawn>, FNF_MonsterCombatSlotReservation>& Pair
			: MonsterCombatSlotReservations)
		{
			if (!Pair.Key.IsValid()
				|| Pair.Key.Get() == RequestingMonster
				|| Pair.Value.TargetActor.Get() != TargetActor)
			{
				continue;
			}

			const AAIController* ReservedMonsterController =
				Cast<AAIController>(Pair.Key->GetController());
			const UBlackboardComponent* ReservedMonsterBlackboard =
				ReservedMonsterController
					? ReservedMonsterController->GetBlackboardComponent()
					: nullptr;

			if (!ReservedMonsterBlackboard
				|| ReservedMonsterBlackboard->GetValueAsObject(TargetActorKeyName)
					!= TargetActor)
			{
				continue;
			}

			const FVector ReservedSlotLocation =
				ReservedMonsterBlackboard->IsVectorValueSet(
					CombatSlotLocationKeyName)
					? ReservedMonsterBlackboard->GetValueAsVector(
						CombatSlotLocationKeyName)
					: Pair.Value.Location;

			if (FVector::DistSquared2D(
				CandidateLocation,
				ReservedSlotLocation) < FMath::Square(MinimumSlotSeparation))
			{
				return true;
			}
		}

		return false;
	}
}

UBTTask_NF_MonsterReserveCombatSlot::UBTTask_NF_MonsterReserveCombatSlot()
{
	NodeName = TEXT("Monster Reserve Combat Slot");
}

void UBTTask_NF_MonsterReserveCombatSlot::ReleaseReservation(
	APawn* MonsterPawn)
{
	if (!IsValid(MonsterPawn))
	{
		return;
	}

	MonsterCombatSlotReservations.Remove(
		TWeakObjectPtr<APawn>(MonsterPawn));
}

EBTNodeResult::Type UBTTask_NF_MonsterReserveCombatSlot::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	AAIController* MonsterAIController = OwnerComp.GetAIOwner();
	APawn* MonsterPawn = MonsterAIController
		? MonsterAIController->GetPawn()
		: nullptr;
	UBlackboardComponent* BlackboardComponent =
		OwnerComp.GetBlackboardComponent();

	if (!MonsterPawn || !MonsterPawn->HasAuthority() || !BlackboardComponent)
	{
		return EBTNodeResult::Failed;
	}

	AActor* TargetActor = Cast<AActor>(
		BlackboardComponent->GetValueAsObject(TargetActorKeyName));

	if (!TargetActor)
	{
		return EBTNodeResult::Failed;
	}

	if (const FNF_MonsterCombatSlotReservation* ExistingReservation =
		MonsterCombatSlotReservations.Find(MonsterPawn))
	{
		if (ExistingReservation->TargetActor.Get() == TargetActor)
		{
			BlackboardComponent->SetValueAsVector(
				CombatSlotLocationKeyName,
				ExistingReservation->Location);
			return EBTNodeResult::Succeeded;
		}
	}

	FVector ChosenSlotLocation = BlackboardComponent->GetValueAsVector(
		CombatSlotLocationKeyName);

	if (IsCombatSlotReserved(
		MonsterPawn,
		TargetActor,
		ChosenSlotLocation,
		MinimumSlotSeparation))
	{
		UNavigationSystemV1* NavigationSystem =
			FNavigationSystem::GetCurrent<UNavigationSystemV1>(MonsterPawn->GetWorld());

		if (!NavigationSystem)
		{
			return EBTNodeResult::Failed;
		}

		const FVector TargetLocation = TargetActor->GetActorLocation();
		const FVector MonsterLocation = MonsterPawn->GetActorLocation();
		const float AngleStep = 360.0f / static_cast<float>(FallbackSlotCount);
		float BestDistanceSquared = TNumericLimits<float>::Max();
		bool bFoundFallbackSlot = false;

		for (int32 SlotIndex = 0; SlotIndex < FallbackSlotCount; ++SlotIndex)
		{
			const float AngleRadians = FMath::DegreesToRadians(
				AngleStep * static_cast<float>(SlotIndex));
			const FVector CandidateLocation = TargetLocation + FVector(
				FMath::Cos(AngleRadians) * FallbackSlotRadius,
				FMath::Sin(AngleRadians) * FallbackSlotRadius,
				0.0f);

			FNavLocation ProjectedLocation;
			if (!NavigationSystem->ProjectPointToNavigation(
				CandidateLocation,
				ProjectedLocation,
				FVector(75.0f, 75.0f, 300.0f))
				|| IsCombatSlotReserved(
					MonsterPawn,
					TargetActor,
					ProjectedLocation.Location,
					MinimumSlotSeparation))
			{
				continue;
			}

			const float DistanceSquared = FVector::DistSquared2D(
				MonsterLocation,
				ProjectedLocation.Location);

			if (DistanceSquared < BestDistanceSquared)
			{
				BestDistanceSquared = DistanceSquared;
				ChosenSlotLocation = ProjectedLocation.Location;
				bFoundFallbackSlot = true;
			}
		}

		if (!bFoundFallbackSlot)
		{
			return EBTNodeResult::Failed;
		}
	}

	MonsterCombatSlotReservations.FindOrAdd(MonsterPawn) = {
		TargetActor,
		ChosenSlotLocation
	};
	BlackboardComponent->SetValueAsVector(
		CombatSlotLocationKeyName, ChosenSlotLocation);
	BlackboardComponent->SetValueAsVector(
		CombatSlotAnchorLocationKeyName,
		TargetActor->GetActorLocation());

	return EBTNodeResult::Succeeded;
}
