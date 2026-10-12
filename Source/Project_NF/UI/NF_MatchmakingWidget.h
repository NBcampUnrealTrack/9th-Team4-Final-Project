// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/Match/NF_MatchRules.h"
#include "NF_MatchmakingWidget.generated.h"

class UButton;
class UTextBlock;
class UNF_MatchmakingSubsystem;

/** Lightweight native UMG panel for joining and leaving the Steam matchmaking queue. */
UCLASS()
class UNF_MatchmakingWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void BuildWidgetTree();
	void RefreshDisplay();
	void RefreshSteamStatus();
	void LogSteamStartupDiagnostics(const FString& DefaultSubsystemName);
	void SetSteamStatus(const FString& StatusText, const FLinearColor& Color, bool bIsError);

	UFUNCTION()
	void HandleMatchStateChanged(ENFMatchState NewState);

	UFUNCTION()
	void HandleQueueCountChanged(int32 PlayerCount, int32 RequiredPlayers);

	UFUNCTION()
	void HandleMatchmakingFailed(const FString& Reason);

	UFUNCTION()
	void HandleStartButtonClicked();

	UFUNCTION()
	void HandleCancelButtonClicked();

	UPROPERTY(Transient)
	TObjectPtr<UButton> StartButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CancelButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StateText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SteamStatusText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PlayerCountText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ElapsedTimeText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FeedbackText;

	UPROPERTY(Transient)
	TObjectPtr<UNF_MatchmakingSubsystem> MatchmakingSubsystem;

	FTimerHandle DisplayRefreshTimerHandle;
	FString LastLoggedSteamStatus;
	bool bLoggedSteamStartupDiagnostics = false;
};
