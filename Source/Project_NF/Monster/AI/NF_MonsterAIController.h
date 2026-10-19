#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "NF_MonsterAIController.generated.h"

class AActor;
class APawn;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UBehaviorTree;

UCLASS()
class PROJECT_NF_API ANF_MonsterAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	ANF_MonsterAIController();
	
	AActor* GetCurrentTargetActor() const;
	
	void SetCurrentTargetActor(AActor* NewTargetActor);

	void ClearCurrentTargetActor();
	
	void UpdateLastKnownLocation();
	
protected:
	virtual void OnPossess(APawn* InPawn) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Monster|AI")
	TObjectPtr<UAIPerceptionComponent> MonsterPerceptionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Monster|AI")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Monster|AI")
	TObjectPtr<UBehaviorTree> MonsterBehaviorTree;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Monster|AI")
	FName TargetActorKeyName = TEXT("TargetActor");
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Monster|AI")
	FName HasLineOfSightKeyName = TEXT("HasLineOfSight");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Monster|AI")
	FName LastKnownLocationKeyName = TEXT("LastKnownLocation");

	UFUNCTION()
	void HandleTargetPerceptionUpdated(
		AActor* UpdatedActor,
		FAIStimulus Stimulus);
};
