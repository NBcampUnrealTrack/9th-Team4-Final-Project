
#pragma once

#include "CoreMinimal.h"
#include "Data/Struct/NF_MatchState.h"
#include "GameFramework/GameState.h"
#include "NF_GameState.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FNF_OnMatchStateChanged, FNF_MatchState&);

UCLASS()
class PROJECT_NF_API ANF_GameState : public AGameState
{
	GENERATED_BODY()

public:
	ANF_GameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "NF|Match")
	FNF_MatchState GetMatchState() const;

	UFUNCTION(BlueprintPure, Category = "NF|Match")
	ENF_MatchPhase GetMatchPhase() const;

	UFUNCTION(BlueprintPure, Category = "NF|Match")
	double GetRemainingPhaseTime() const;

	//서버권한으로만 호출
	bool AuthoritySetTimedMatchPhase(ENF_MatchPhase NewPhase,float Duration);
	//제한시간 없는 페이즈용
	bool AuthoritySetUntimedMatchPhase(ENF_MatchPhase NewPhase);
	
	FNF_OnMatchStateChanged OnMatchStateChanged;

private:
	UPROPERTY(ReplicatedUsing = OnRep_NFMatchState)
	FNF_MatchState NF_MatchState;
	
	UFUNCTION()
	void OnRep_NFMatchState();
	
	void BroadcastMatchStateChanged();
};