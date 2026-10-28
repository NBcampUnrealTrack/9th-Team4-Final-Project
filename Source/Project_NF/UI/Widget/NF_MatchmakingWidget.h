// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NF_MatchmakingWidget.generated.h"

class UButton;
class UTextBlock;
class UNF_MatchmakingViewModel;
class UNF_ViewModelSubsystem;

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
	void DisconnectViewModel();
	void RefreshSteamStatus();
	void LogSteamStartupDiagnostics(const FString& DefaultSubsystemName);
	void SetSteamStatus(const FString& StatusText, const FLinearColor& Color, bool bIsError);

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
	TObjectPtr<UNF_MatchmakingViewModel> MatchmakingViewModel;

	TWeakObjectPtr<UNF_ViewModelSubsystem> ViewModelSubsystem;
	FString LastLoggedSteamStatus;
	bool bLoggedSteamStartupDiagnostics = false;
};
