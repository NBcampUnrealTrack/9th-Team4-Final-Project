// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Base/NF_ViewModelBase.h"
#include "Core/Match/NF_MatchRules.h"
#include "NF_MatchmakingViewModel.generated.h"


class UNF_MatchmakingSubsystem;

UENUM(BlueprintType)
enum class ENFSteamConnectionState : uint8
{
	Unavailable,
	IdentityUnavailable,
	Offline,
	MissingUserId,
	Connected
};

UCLASS(BlueprintType)
class PROJECT_NF_API UNF_MatchmakingViewModel : public UNF_ViewModelBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Matchmaking")
	bool StartMatchmaking();

	UFUNCTION(BlueprintCallable, Category = "Matchmaking")
	void CancelMatchmaking();

	UFUNCTION(BlueprintPure, FieldNotify, Category = "Matchmaking|UI")
	FText GetMatchStateText() const;

	UFUNCTION(BlueprintPure, FieldNotify, Category = "Matchmaking|UI")
	FText GetPlayerCountText() const;

	UFUNCTION(BlueprintPure, FieldNotify, Category = "Matchmaking|UI")
	FText GetElapsedTimeText() const;

	UFUNCTION(BlueprintPure, FieldNotify, Category = "Matchmaking|UI")
	bool GetCanStartMatchmaking() const;

	UFUNCTION(BlueprintPure, FieldNotify, Category = "Matchmaking|UI")
	bool GetCanCancelMatchmaking() const;

protected:
	virtual void OnInitializeViewModel() override;
	virtual void OnViewModelContextChanged() override;
	virtual void OnDeinitializeViewModel() override;

private:
	void BindMatchmakingSubsystem();
	void UnbindMatchmakingSubsystem();

	void StartRefreshTimer();
	void StopRefreshTimer();

	void RefreshAll();
	void RefreshElapsedTime();
	void RefreshSteamStatus();

	int32 ResolveRequiredPlayerCount() const;

	void UpdateMatchState(ENFMatchState NewState);
	void UpdateQueueCount(int32 NewQueuedPlayerCount,int32 NewRequiredPlayerCount);

	void UpdateSteamStatus(ENFSteamConnectionState NewState,const FText& NewStatusText);

	void SetFailureMessage(const FText& NewFailureMessage);

	UFUNCTION()
	void HandleMatchStateChanged(ENFMatchState NewState);

	UFUNCTION()
	void HandleQueueCountChanged(int32 PlayerCount,int32 RequiredPlayers);

	UFUNCTION()
	void HandleMatchmakingFailed(const FString& Reason);

	UPROPERTY(BlueprintReadOnly,FieldNotify,Category = "Matchmaking|UI",meta = (AllowPrivateAccess = "true"))
	ENFMatchState MatchState = ENFMatchState::Ended;

	UPROPERTY(BlueprintReadOnly,FieldNotify,Category = "Matchmaking|UI",meta = (AllowPrivateAccess = "true"))
	int32 QueuedPlayerCount = 0;

	UPROPERTY(BlueprintReadOnly,FieldNotify,Category = "Matchmaking|UI",meta = (AllowPrivateAccess = "true"))
	int32 RequiredPlayerCount = NF_MatchRules::PlayerCount;

	UPROPERTY(BlueprintReadOnly,FieldNotify,Category = "Matchmaking|UI",meta = (AllowPrivateAccess = "true"))
	int32 ElapsedSeconds = 0;

	UPROPERTY(BlueprintReadOnly,FieldNotify,Category = "Matchmaking|UI",meta = (AllowPrivateAccess = "true"))
	ENFSteamConnectionState SteamConnectionState =ENFSteamConnectionState::Unavailable;

	UPROPERTY(BlueprintReadOnly,FieldNotify,Category = "Matchmaking|UI",meta = (AllowPrivateAccess = "true"))
	FText SteamStatusText;

	UPROPERTY(BlueprintReadOnly,FieldNotify,Category = "Matchmaking|UI",meta = (AllowPrivateAccess = "true"))
	FText FailureMessage;

	UPROPERTY(Transient)
	TObjectPtr<UNF_MatchmakingSubsystem> MatchmakingSubsystem;

	TWeakObjectPtr<UWorld> RefreshTimerWorld;
	FTimerHandle RefreshTimerHandle;
};
