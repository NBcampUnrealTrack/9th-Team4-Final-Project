#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NF_EscapeComponent.generated.h"

class ANF_EscapeZone;
class ANF_PlayerState;
class APawn;

DECLARE_DELEGATE_RetVal_ThreeParams(bool, FNF_ValidateEscapeRequest, ANF_EscapeZone*, ANF_PlayerState*, ANF_PlayerState*&);
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

	// GameMode가 열쇠와 매치 정보를 검증한다. 열쇠 컴포넌트를 직접 참조하지 않는다.
	FNF_ValidateEscapeRequest ValidateEscapeRequest;
	FNF_OnEscapeSucceeded OnEscapeSucceeded;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool HasServerAuthority() const;
	bool ValidateActiveEscape() const;
	void MonitorEscape();
	void CompleteEscape();

	TWeakObjectPtr<ANF_EscapeZone> ActiveZone;
	TWeakObjectPtr<ANF_PlayerState> ActiveRequester;
	TWeakObjectPtr<ANF_PlayerState> StartingHolder;

	int32 EscapingTeamId = INDEX_NONE;
	double EndServerTime = 0.0;
	bool bEscaping = false;

	FTimerHandle MonitorTimerHandle;
	FTimerHandle CompletionTimerHandle;
};
