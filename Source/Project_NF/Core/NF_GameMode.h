#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Core/NF_CoreIntegrationTypes.h"
#include "Data/Struct/NF_EscapeState.h"
#include "Data/Struct/NF_MatchResult.h"
#include "NF_GameMode.generated.h"

class ANF_GameState;
class APlayerController;
class ANF_PlayerState;
class ANF_BossBase;
class ANF_PlayerController;
class ANF_EscapeZone;
class UNF_EscapeComponent;
class UNF_EscapeKeyComponent;
class UNF_BuildReadyComponent;
class UNF_MatchDepartureComponent;
class UNF_BossEncounterComponent;
class UNF_BossResolutionComponent;
class UNF_BossContributionRewardComponent;

struct FNFServerTeamIdAssignment;
struct FNF_BossDefeatResult;
struct FNF_KeyPublicState;

UCLASS()
class PROJECT_NF_API ANF_GameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ANF_GameMode();

	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 서버에서 팀 배정과 PlayerState 반영을 완료한 뒤 호출
	void NotifyTeamsReady();

	// 보스 시스템이 보스 처치 결과를 Core에 전달
	bool NotifyBossDefeated(const FNF_BossDefeatResult& DefeatResult);

	// 탈출 시스템이 서버에서 터치다운 조건을 검증한 뒤 호출
	void NotifyEscapeSucceeded(int32 WinningTeamId);

	UNF_BuildReadyComponent* GetBuildReadyComponent() const;

	FNF_RequestMatchAbort RequestMatchAbort;


	// BuildSystem이 서버에서 기본 빌드 적용 함수를 등록한다.
	bool RegisterFallbackBuildHandler(const FNF_ApplyFallbackBuild& Handler);

	// 자신이 등록한 함수만 해제할 수 있다.
	void UnregisterFallbackBuildHandler(const UObject* HandlerOwner);

	bool RegisterBossSpawnHandler(const FNF_RequestBossSpawn& Handler);

	void UnregisterBossSpawnHandler(const UObject* HandlerOwner);

private:
	UPROPERTY(EditDefaultsOnly, Category = "NF|Match")
	int32 InitialRequiredPlayers = 8;

	int32 CurrentPlayerCount = 0;

	UPROPERTY(EditDefaultsOnly, Category = "NF|Match")
	float BuildDuration = 60.0f;

	UPROPERTY(EditDefaultsOnly, Category = "NF|Match")
	float FarmingDuration = 900.0f;

	ANF_GameState* GetNFGameState() const;

	bool CollectMatchParticipants(TArray<ANF_PlayerState*>& OutParticipants) const;

	void TryStartBuild();
	void EnterBuild();
	void HandleBuildPhaseExpired();
	void EnterFarming();
	void EnterBoss();
	void EnterEscape();
	bool FinishMatch(const FNF_MatchResult& Result);

	void AbortMatchDueToDepartures();

	void HandleBuildResolveSucceeded();
	void HandleBuildResolveFailed(const FString& Reason);

	// 이탈 실패와 빌드 실패가 공유하는 매치 중단 처리
	void AbortMatch(const FString& Reason,ENF_MatchEndReason EndReason = ENF_MatchEndReason::RuntimeFailure);

	void HandleServerTeamIdsAssigned(const TArray<FNFServerTeamIdAssignment>& Assignments);
#pragma region component

private:
	UPROPERTY(VisibleAnywhere, Category = "NF|Match|Build")
	TObjectPtr<UNF_BuildReadyComponent> BuildReadyComponent;

	UPROPERTY(VisibleAnywhere, Category = "NF|Match|Departure")
	TObjectPtr<UNF_MatchDepartureComponent> MatchDepartureComponent;

	UPROPERTY(VisibleAnywhere, Category = "NF|Match|Departure")
	TObjectPtr<UNF_BossEncounterComponent> BossEncounterComponent;

	UPROPERTY(VisibleAnywhere, Category = "NF|Match|Boss")
	TObjectPtr<UNF_BossResolutionComponent> BossResolutionComponent;

	UPROPERTY(VisibleAnywhere, Category = "NF|Match|Boss")
	TObjectPtr<UNF_BossContributionRewardComponent> BossContributionRewardComponent;

	UPROPERTY(VisibleAnywhere, Category = "NF|Match|Escape")
	TObjectPtr<UNF_EscapeKeyComponent> EscapeKeyComponent;

	UPROPERTY(VisibleAnywhere, Category = "NF|Match|Escape")
	TObjectPtr<UNF_EscapeComponent> EscapeComponent;

	bool bTeamsReady = false;

	bool bMatchAborting = false;
#pragma endregion

#pragma region BossPhase

public:
	UNF_BossEncounterComponent* GetBossEncounterComponent() const;
	UNF_BossResolutionComponent* GetBossResolutionComponent() const;
	UNF_BossContributionRewardComponent* GetBossContributionRewardComponent() const;
	UNF_EscapeKeyComponent* GetEscapeKeyComponent() const;

private:
	void HandleFarmingPhaseExpired();

	void HandleBossSpawnSucceeded(ANF_BossBase* Boss);
	void HandleBossSpawnFailed(const FString& Reason);

	void HandleBossDefeatResolved(const FNF_BossDefeatResult& DefeatResult);

	FTimerHandle PhaseTimerHandle;
#pragma endregion

#pragma region Escape

public:
	bool RequestEscape(ANF_EscapeZone* Zone, ANF_PlayerState* Requester);
	void NotifyEscapeRangeExited(ANF_EscapeZone* Zone, ANF_PlayerState* PlayerState);

	FNF_OnMatchPlayerLeaving OnMatchPlayerLeaving;
	FNF_OnMatchCleanup OnMatchCleanup;

private:
	FNF_EscapeValidationResult EvaluateEscapeRequest(ANF_EscapeZone* Zone, ANF_PlayerState* Requester);
	bool CanPlayerUseEscape(const ANF_PlayerState* PlayerState) const;
	void NotifyMatchCleanup();
	void HandleInitialKeyAssignmentFailed(const FString& Reason);
	void HandleKeyOperationFailed(const FString& Reason);
	void HandleKeyStateChanged(const FNF_KeyPublicState& NewState);
	void SyncKeyMinimapForPlayer(ANF_PlayerController* PlayerController);

	FGuid KeyRevealSourceId;
	
	bool bMatchCleanupNotified = false;

#pragma endregion

#pragma region waiting

public:
	FNF_RequestMatchCompleted RequestMatchCompleted;

private:
	UPROPERTY(EditDefaultsOnly,Category = "NF|Match|Waiting",meta = (ClampMin = "1.0"))
	float WaitingDuration = 90.0f;

	UPROPERTY(EditDefaultsOnly,Category = "NF|Match|Result",meta = (ClampMin = "0.1"))
	float ResultDisplayDuration = 5.0f;

	void StartWaiting();
	void HandleWaitingPhaseExpired();

	
	void DispatchMatchConclusion();

	bool bWaitingStarted = false;
	bool bMatchFinalized = false;
	bool bConclusionDispatched = false;

	FTimerHandle ResultDisplayTimerHandle;
#pragma endregion
};
