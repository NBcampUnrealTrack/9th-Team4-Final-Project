// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SlateWrapperTypes.h"
#include "Data/Enum/NF_MatchPhase.h"
#include "UI/ViewModel/Base/NF_ViewModelBase.h"
#include "NF_GameFlowViewModel.generated.h"

struct FNF_MatchState;
class ANF_GameState;

USTRUCT()
struct FNF_QueuedNotification
{
	GENERATED_BODY()

	FText Message;
	float DisplayDuration = 3.0f;
};

UCLASS()
class PROJECT_NF_API UNF_GameFlowViewModel : public UNF_ViewModelBase
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "GameFlow|Notification")
	void EnqueueNotification(const FText& Message,float DisplayDuration = 3.0f);

	UFUNCTION(BlueprintPure, FieldNotify, Category = "GameFlow|Phase")
	FText GetPhaseText() const;

	UFUNCTION(BlueprintPure, FieldNotify, Category = "GameFlow|Phase")
	FText GetRemainingTimeText() const;

	UFUNCTION(BlueprintPure, FieldNotify, Category = "GameFlow|Phase")
	ESlateVisibility GetPhaseTimerVisibility() const;

	UFUNCTION(BlueprintPure, FieldNotify, Category = "GameFlow|Notification")
	ESlateVisibility GetNotificationVisibility() const;

protected:
	virtual void OnInitializeViewModel() override;
	virtual void OnViewModelContextChanged() override;
	virtual void OnDeinitializeViewModel() override;

private:
	void TryBindGameState();
	void UnbindGameState();

	void StartRefreshTimer();
	void StopRefreshTimer();
	void RefreshViewModel();

	void RefreshFromGameState();
	void RefreshRemainingTime();

	void ShowPhaseNotification(ENF_MatchPhase NewPhase);
	void ShowNotification(const FNF_QueuedNotification& Notification);
	void UpdateNotification();
	void ShowNextNotification();
	void ClearNotification();

	void HandleMatchStateChanged(FNF_MatchState& NewMatchState);

	UPROPERTY(BlueprintReadOnly,FieldNotify,Category = "GameFlow|Phase",meta = (AllowPrivateAccess = "true"))
	ENF_MatchPhase CurrentPhase = ENF_MatchPhase::Waiting;

	UPROPERTY(BlueprintReadOnly,FieldNotify,Category = "GameFlow|Phase",meta = (AllowPrivateAccess = "true"))
	bool bShowPhaseTimer = false;

	UPROPERTY(BlueprintReadOnly,FieldNotify,Category = "GameFlow|Phase",meta = (AllowPrivateAccess = "true"))
	int32 RemainingPhaseSeconds = 0;

	UPROPERTY(BlueprintReadOnly,FieldNotify,Category = "GameFlow|Notification",meta = (AllowPrivateAccess = "true"))
	FText NotificationText;

	UPROPERTY(BlueprintReadOnly,FieldNotify,Category = "GameFlow|Notification",meta = (AllowPrivateAccess = "true"))
	bool bShowNotification = false;

	UPROPERTY(Transient)
	TObjectPtr<ANF_GameState> GameState;

	TArray<FNF_QueuedNotification> PendingNotifications;

	TWeakObjectPtr<UWorld> RefreshTimerWorld;
	
	FTimerHandle RefreshTimerHandle;

	double NotificationEndWorldTime = 0.0;
};
