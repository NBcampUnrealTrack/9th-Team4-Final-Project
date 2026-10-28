
#pragma once

#include "CoreMinimal.h"
#include "Data/Struct/NF_MatchState.h"
#include "GameFramework/GameState.h"
#include "Data/Enum/NF_BuildShopType.h"
#include "Data/Struct/NF_EscapeState.h"
#include "Data/Struct/NF_KeyPublicState.h"

#include "NF_GameState.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FNF_OnMatchStateChanged, FNF_MatchState&);
DECLARE_MULTICAST_DELEGATE_OneParam(FNF_OnKeyPublicStateChanged, const FNF_KeyPublicState&);
DECLARE_MULTICAST_DELEGATE_OneParam(FNF_OnEscapeProgressChanged, const FNF_EscapeProgress&);
DECLARE_MULTICAST_DELEGATE(FNF_OnTeamRosterChanged);
DECLARE_MULTICAST_DELEGATE_TwoParams(FNF_OnAnnouncementReceived, const FText&, float);

UCLASS()
class PROJECT_NF_API ANF_GameState : public AGameState
{
	GENERATED_BODY()

public:
	ANF_GameState();

	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;

	// 서버와 클라이언트에서 현재 명단 또는 팀 ID가 변경된 후 알린다.
	FNF_OnTeamRosterChanged OnTeamRosterChanged;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "NF|Match")
	FNF_MatchState GetMatchState() const;

	UFUNCTION(BlueprintPure, Category = "NF|Match")
	ENF_MatchPhase GetMatchPhase() const;

	UFUNCTION(BlueprintPure, Category = "NF|Match")
	double GetRemainingPhaseTime() const;

	UFUNCTION(BlueprintPure, Category = "NF|Match")
	double GetRemainingConclusionTime() const;

	//서버권한으로만 호출
	bool AuthoritySetTimedMatchPhase(ENF_MatchPhase NewPhase,float Duration);
	//제한시간 없는 페이즈용
	bool AuthoritySetUntimedMatchPhase(ENF_MatchPhase NewPhase);
	// 결과 기록과 Finished 전환을 한 번에 수행.
	bool AuthorityFinishMatch(const FNF_MatchResult& Result, float ConclusionDuration);
	
	FNF_OnMatchStateChanged OnMatchStateChanged;

	UFUNCTION(BlueprintPure, Category = "NF|Escape|Key")
	FNF_KeyPublicState GetKeyPublicState() const;

	bool AuthoritySetKeyPublicState(const FNF_KeyPublicState& NewState);

	FNF_OnKeyPublicStateChanged OnKeyPublicStateChanged;

	UFUNCTION(BlueprintPure, Category = "NF|Escape")
	FNF_EscapeProgress GetEscapeProgress() const;

	UFUNCTION(BlueprintPure, Category = "NF|Escape")
	float GetRemainingEscapeTime() const;

	bool AuthoritySetEscapeProgress(const FNF_EscapeProgress& NewProgress);

	FNF_OnEscapeProgressChanged OnEscapeProgressChanged;

	// 각 클라이언트에서 수신한 일회성 진행 알림.
	FNF_OnAnnouncementReceived OnAnnouncementReceived;

	// 서버에서 호출하여 현재 접속 중인 클라이언트에 알림 전달.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastShowAnnouncement(const FText& Message, float Duration = 3.0f);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandlePlayerTeamChanged();

	UPROPERTY(ReplicatedUsing = OnRep_NFMatchState)
	FNF_MatchState NF_MatchState;

	UPROPERTY(ReplicatedUsing = OnRep_KeyPublicState)
	FNF_KeyPublicState KeyPublicState;

	UPROPERTY(ReplicatedUsing = OnRep_EscapeProgress)
	FNF_EscapeProgress EscapeProgress;

	UFUNCTION()
	void OnRep_KeyPublicState();

	UFUNCTION()
	void OnRep_EscapeProgress();
	
	UFUNCTION()
	void OnRep_NFMatchState();
	
	void BroadcastMatchStateChanged();
	
#pragma region BuildSystem
public:
	//현재 페이즈에서 해당 빌드상점이 허용되는지 조회 (사망여부에 따른 상점이용제한은 태그를 사용해 구현할 것)
	UFUNCTION(BlueprintPure, Category = "NF|Match|Build")
	bool IsBuildShopAllowed(ENF_BuildShopType ShopType) const;
#pragma endregion
};
