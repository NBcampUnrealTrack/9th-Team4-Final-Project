// Fill out your copyright notice in the Description page of Project Settings.


#include "NF_GameFlowViewModel.h"

#include "Core/NF_GameState.h"

void UNF_GameFlowViewModel::OnInitializeViewModel()
{
	Super::OnInitializeViewModel();

	TryBindGameState();
	StartRefreshTimer();
	RefreshViewModel();
}

void UNF_GameFlowViewModel::OnViewModelContextChanged()
{
	Super::OnViewModelContextChanged();

	UnbindGameState();
	StopRefreshTimer();

	TryBindGameState();
	StartRefreshTimer();
	RefreshViewModel();
}

void UNF_GameFlowViewModel::OnDeinitializeViewModel()
{
	StopRefreshTimer();
	UnbindGameState();

	PendingNotifications.Reset();
	ClearNotification();

	Super::OnDeinitializeViewModel();
}

void UNF_GameFlowViewModel::TryBindGameState()
{
	ULocalPlayer* LocalPlayer =GetViewModelContext().LocalPlayer.Get();
	if (!IsValid(LocalPlayer))return;

	UWorld* World = LocalPlayer->GetWorld();
	if (!IsValid(World))return;

	ANF_GameState* NewGameState =World->GetGameState<ANF_GameState>();
	if (GameState == NewGameState)return;

	UnbindGameState();

	GameState = NewGameState;
	if (!IsValid(GameState))return;
	

	GameState->OnMatchStateChanged.AddUObject(this,&ThisClass::HandleMatchStateChanged);

	RefreshFromGameState();
}

void UNF_GameFlowViewModel::UnbindGameState()
{
	if (IsValid(GameState))
	{
		GameState->OnMatchStateChanged.RemoveAll(this);
	}

	GameState = nullptr;
}

void UNF_GameFlowViewModel::StartRefreshTimer()
{
	ULocalPlayer* LocalPlayer =GetViewModelContext().LocalPlayer.Get();
	if (!IsValid(LocalPlayer))return;
	
	UWorld* World = LocalPlayer->GetWorld();
	if (!IsValid(World))return;

	RefreshTimerWorld = World;
	
	World->GetTimerManager().SetTimer(
		RefreshTimerHandle,
		this,
		&ThisClass::RefreshViewModel,
		0.1f,
		true);
}

void UNF_GameFlowViewModel::StopRefreshTimer()
{
	UWorld* World = RefreshTimerWorld.Get();

	if (IsValid(World))
	{
		World->GetTimerManager().ClearTimer(RefreshTimerHandle);
	}

	RefreshTimerHandle.Invalidate();
	RefreshTimerWorld.Reset();
}

void UNF_GameFlowViewModel::RefreshViewModel()
{
	//GameState가 늦게 생성될 경우 사용
	TryBindGameState();

	RefreshRemainingTime();
	UpdateNotification();
}

void UNF_GameFlowViewModel::RefreshFromGameState()
{
	if (!IsValid(GameState))return;
	
	const FNF_MatchState MatchState =GameState->GetMatchState();

	if (UE_MVVM_SET_PROPERTY_VALUE(CurrentPhase,MatchState.Phase))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetPhaseText);
	}

	if (UE_MVVM_SET_PROPERTY_VALUE(bShowPhaseTimer,MatchState.bHasTimeLimit))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetPhaseTimerVisibility);
	}

	RefreshRemainingTime();
}

void UNF_GameFlowViewModel::RefreshRemainingTime()
{
	int32 NewRemainingSeconds = 0;

	if (IsValid(GameState))
	{
		const FNF_MatchState MatchState =GameState->GetMatchState();

		if (MatchState.bHasTimeLimit)
		{
			NewRemainingSeconds =FMath::CeilToInt(GameState->GetRemainingPhaseTime());
		}
	}

	NewRemainingSeconds =FMath::Max(0, NewRemainingSeconds);

	if (UE_MVVM_SET_PROPERTY_VALUE(RemainingPhaseSeconds,NewRemainingSeconds))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetRemainingTimeText);
	}
}

void UNF_GameFlowViewModel::HandleMatchStateChanged(FNF_MatchState& NewMatchState)
{
	const bool bPhaseChanged =CurrentPhase != NewMatchState.Phase;

	if (UE_MVVM_SET_PROPERTY_VALUE(CurrentPhase,NewMatchState.Phase))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetPhaseText);
	}

	if (UE_MVVM_SET_PROPERTY_VALUE(bShowPhaseTimer,NewMatchState.bHasTimeLimit))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetPhaseTimerVisibility);
	}

	RefreshRemainingTime();

	if (bPhaseChanged)
	{
		ShowPhaseNotification(NewMatchState.Phase);
	}
}

void UNF_GameFlowViewModel::ShowPhaseNotification(const ENF_MatchPhase NewPhase)
{
	FText Message;

	switch (NewPhase)
	{
	case ENF_MatchPhase::Waiting:Message = FText::FromString(TEXT("플레이어를 기다리는 중입니다."));
		break;

	case ENF_MatchPhase::Build:Message = FText::FromString(TEXT("장비 및 스킬을 설정하세요."));
		break;

	case ENF_MatchPhase::Farming:Message = FText::FromString(TEXT("성장 및 파밍 페이즈가 시작되었습니다."));
		break;

	case ENF_MatchPhase::Boss:Message = FText::FromString(TEXT("보스가 출현했습니다."));
		break;

	case ENF_MatchPhase::Escape:Message = FText::FromString(TEXT("탈출구를 통해 탈출하세요."));
		break;

	case ENF_MatchPhase::Finished:Message = FText::FromString(TEXT("게임이 종료되었습니다."));
		break;

	default:
		return;
	}

	EnqueueNotification(Message, 3.0f);
}

void UNF_GameFlowViewModel::EnqueueNotification(const FText& Message,const float DisplayDuration)
{
	if (Message.IsEmpty()) return;

	FNF_QueuedNotification Notification;
	Notification.Message = Message;
	Notification.DisplayDuration =FMath::Max(0.1f, DisplayDuration);

	if (!bShowNotification)
	{
		ShowNotification(Notification);
		return;
	}

	PendingNotifications.Add(Notification);
}

void UNF_GameFlowViewModel::ShowNotification(const FNF_QueuedNotification& Notification)
{
	UWorld* World = RefreshTimerWorld.Get();

	if (!IsValid(World))return;

	UE_MVVM_SET_PROPERTY_VALUE(NotificationText,Notification.Message);
	if (UE_MVVM_SET_PROPERTY_VALUE(bShowNotification,true))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetNotificationVisibility);
	}

	NotificationEndWorldTime =World->GetTimeSeconds()+ Notification.DisplayDuration;
}

void UNF_GameFlowViewModel::UpdateNotification()
{
	if (!bShowNotification)return;

	UWorld* World = RefreshTimerWorld.Get();

	if (!IsValid(World))
	{
		ClearNotification();
		return;
	}

	if (World->GetTimeSeconds() < NotificationEndWorldTime) return;
	
	ShowNextNotification();
}

void UNF_GameFlowViewModel::ShowNextNotification()
{
	if (PendingNotifications.IsEmpty())
	{
		ClearNotification();
		return;
	}

	const FNF_QueuedNotification NextNotification =PendingNotifications[0];

	PendingNotifications.RemoveAt(0);

	ShowNotification(NextNotification);
}

void UNF_GameFlowViewModel::ClearNotification()
{
	NotificationEndWorldTime = 0.0;

	UE_MVVM_SET_PROPERTY_VALUE(NotificationText,FText::GetEmpty());
	if (UE_MVVM_SET_PROPERTY_VALUE(bShowNotification,false))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetNotificationVisibility);
	}
}

FText UNF_GameFlowViewModel::GetPhaseText() const
{
	switch (CurrentPhase)
	{
	case ENF_MatchPhase::Waiting:return FText::FromString(TEXT("플레이어 대기"));

	case ENF_MatchPhase::Build:return FText::FromString(TEXT("장비 설정"));

	case ENF_MatchPhase::Farming:return FText::FromString(TEXT("성장 및 파밍"));

	case ENF_MatchPhase::Boss:return FText::FromString(TEXT("보스 전투"));

	case ENF_MatchPhase::Escape:return FText::FromString(TEXT("탈출"));

	case ENF_MatchPhase::Finished:return FText::FromString(TEXT("게임 종료"));

	default:
		return FText::GetEmpty();
	}
}

FText UNF_GameFlowViewModel::GetRemainingTimeText() const
{
	const int32 Minutes =RemainingPhaseSeconds / 60;
	const int32 Seconds =RemainingPhaseSeconds % 60;

	return FText::FromString(FString::Printf(TEXT("%02d:%02d"),Minutes,Seconds));
}

ESlateVisibility UNF_GameFlowViewModel::GetPhaseTimerVisibility() const
{
	return bShowPhaseTimer ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
}

ESlateVisibility UNF_GameFlowViewModel::GetNotificationVisibility() const
{
	return bShowNotification ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
}
