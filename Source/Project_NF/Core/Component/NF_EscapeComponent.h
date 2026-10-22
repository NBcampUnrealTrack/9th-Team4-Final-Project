#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Data/Struct/NF_EscapeState.h"
#include "NF_EscapeComponent.generated.h"

class ANF_EscapeZone;
class ANF_PlayerState;
class APawn;
class UAbilitySystemComponent;

DECLARE_DELEGATE_RetVal_TwoParams(FNF_EscapeValidationResult, FNF_EvaluateEscapeRequest, ANF_EscapeZone*, ANF_PlayerState*);
DECLARE_MULTICAST_DELEGATE_OneParam(FNF_OnEscapeSucceeded, int32);

UCLASS(ClassGroup = (NF))
class PROJECT_NF_API UNF_EscapeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNF_EscapeComponent();

	bool TryStartEscape(ANF_EscapeZone* Zone, ANF_PlayerState* Requester);
	void CancelEscape();
	void HandlePlayerLeaving(ANF_PlayerState* PlayerState, APawn* LastPawn);
	void HandleRangeExited(ANF_EscapeZone* Zone, ANF_PlayerState* PlayerState);

	// GameMode가 열쇠, 팀원 생존 상태와 범위 정보를 판정한다. 열쇠 컴포넌트를 직접 참조하지 않는다.
	FNF_EvaluateEscapeRequest EvaluateEscapeRequest;
	FNF_OnEscapeSucceeded OnEscapeSucceeded;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool HasServerAuthority() const;
	double GetServerTime() const;
	void EvaluateActiveEscape();
	void MonitorEscape();
	void CompleteEscape();
	void StartCountdown(ENF_EscapeState NewState, float Duration);
	void PauseForTeammate();
	void ResumeWithTeammate();
	void ResetToTeamWaiting();
	void SwitchToSoloCountdown();
	void PublishProgress();
	void BindTeammateDeathEvent();
	void UnbindTeammateDeathEvent();
	void HandleTeammateDeathTagChanged(const FGameplayTag Tag, int32 NewCount);

	TWeakObjectPtr<ANF_EscapeZone> ActiveZone;
	TWeakObjectPtr<ANF_PlayerState> ActiveRequester;
	TWeakObjectPtr<ANF_PlayerState> StartingHolder;
	TWeakObjectPtr<ANF_PlayerState> ActiveTeammate;
	TWeakObjectPtr<UAbilitySystemComponent> TeammateASC;
	FDelegateHandle TeammateDeathTagHandle;

	ENF_EscapeState EscapeState = ENF_EscapeState::Idle;
	int32 EscapingTeamId = INDEX_NONE;
	double EndServerTime = 0.0;
	float TotalDuration = 0.0f;
	float RemainingTime = 0.0f;
	bool bEscaping = false;

	FTimerHandle MonitorTimerHandle;
	FTimerHandle CompletionTimerHandle;
};
