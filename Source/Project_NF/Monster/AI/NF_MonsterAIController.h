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
	
	void ConsiderTargetActor(AActor* CandidateActor);

	/** Applies immediate combat awareness when a player damages this monster. */
	void ApplyDamageAggro(AActor* DamageSource);

	bool HasPersistentDamageAggro() const { return bPersistentDamageAggro; }

	void SetGuardFocusActor(AActor* InGuardFocusActor);

	AActor* GetGuardFocusActor() const;
	
	void SetCurrentTargetActor(AActor* NewTargetActor);

	void ClearCurrentTargetActor(bool bForceClear = false);
	
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Monster|AI")
	FName CombatSlotLocationKeyName = TEXT("CombatSlotLocation");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Monster|AI")
	FName CombatSlotAnchorLocationKeyName = TEXT("CombatSlotAnchorLocation");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Monster|AI")
	FName GuardFocusActorKeyName = TEXT("GuardFocusActor");
	
	// 타깃 변경 직후, 현재 타깃을 최소한 유지할 시간
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "NF|Monster|AI|Targeting",
		meta = (ClampMin = "0.0", Units = "s"))
	float TargetLockDuration = 2.0f;

	// 잠금 시간이 끝난 뒤 새 후보가 이 거리만큼 더 가까울 때만 타깃을 바꾼다.
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "NF|Monster|AI|Targeting",
		meta = (ClampMin = "0.0", Units = "cm"))
	float TargetSwitchDistanceMargin = 250.0f;

	UFUNCTION()
	void HandleTargetPerceptionUpdated(
		AActor* UpdatedActor,
		FAIStimulus Stimulus);
	
private:
	bool IsValidPlayerTarget(AActor* TargetActor) const;

	bool ShouldSwitchTarget(AActor* CandidateActor) const;

	AActor* FindClosestPerceivedPlayer(
		AActor* ExcludedActor) const;

	// 서버 월드 시간 기준으로 타깃 변경이 다시 허용되는 시각
	float TargetLockEndTime = 0.0f;

	// A player that damages the monster remains its target until force-cleared.
	bool bPersistentDamageAggro = false;

	TWeakObjectPtr<AActor> PersistentAggroTarget;
};
