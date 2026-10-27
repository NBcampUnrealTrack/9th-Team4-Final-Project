// Fill out your copyright notice in the Description page of Project Settings.


#include "NF_GameFlowViewModel.h"

#include "Core/NF_GameState.h"
#include "Player/State/NF_PlayerState.h"

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
	GameState->OnEscapeProgressChanged.AddUObject(this,&ThisClass::HandleEscapeProgressChanged);

	RefreshFromGameState();
}

void UNF_GameFlowViewModel::UnbindGameState()
{
	if (IsValid(GameState))
	{
		GameState->OnMatchStateChanged.RemoveAll(this);
		GameState->OnEscapeProgressChanged.RemoveAll(this);
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
	RefreshEscapeRemainingTime();
	RefreshConclusionTime();
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
	RefreshEscapeState();
	RefreshConclusionTime();
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

void UNF_GameFlowViewModel::RefreshEscapeState()
{
	FNF_EscapeProgress Progress;

	if (IsValid(GameState))
	{
		Progress = GameState->GetEscapeProgress();
	}

	if (UE_MVVM_SET_PROPERTY_VALUE(bShowEscapeProgress, Progress.bInProgress))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetEscapeProgressVisibility);
	}

	if (UE_MVVM_SET_PROPERTY_VALUE(bEscapePaused, Progress.bPaused))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetEscapePausedVisibility);
	}

	UE_MVVM_SET_PROPERTY_VALUE(EscapeState, Progress.State);
	// 상태가 같아도 탈출 팀이나 로컬 플레이어 컨텍스트가 바뀔 수 있다.
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetEscapeStatusText);

	if (UE_MVVM_SET_PROPERTY_VALUE(EscapeTotalSeconds, FMath::Max(0.0f, Progress.TotalDuration)))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetEscapeTotalTimeText);
	}

	RefreshEscapeRemainingTime();
}

void UNF_GameFlowViewModel::RefreshEscapeRemainingTime()
{
	float NewRemainingSeconds = 0.0f;

	if (IsValid(GameState) && bShowEscapeProgress)
	{
		NewRemainingSeconds = GameState->GetRemainingEscapeTime();
	}

	NewRemainingSeconds = FMath::Max(0.0f, NewRemainingSeconds);

	if (UE_MVVM_SET_PROPERTY_VALUE(EscapeRemainingSeconds, NewRemainingSeconds))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetEscapeRemainingTimeText);
	}

	float NewProgressPercent = 0.0f;
	if (EscapeTotalSeconds > KINDA_SMALL_NUMBER)
	{
		NewProgressPercent = 1.0f - NewRemainingSeconds / EscapeTotalSeconds;
		NewProgressPercent = FMath::Clamp(NewProgressPercent, 0.0f, 1.0f);
	}

	if (UE_MVVM_SET_PROPERTY_VALUE(EscapeProgressPercent, NewProgressPercent))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetEscapeProgressPercent);
	}
}

void UNF_GameFlowViewModel::RefreshConclusionTime()
{
	bool bNewShowConclusionTimer = false;
	int32 NewRemainingSeconds = 0;

	if (IsValid(GameState))
	{
		const FNF_MatchState MatchState = GameState->GetMatchState();
		bNewShowConclusionTimer = MatchState.Phase == ENF_MatchPhase::Finished &&
			MatchState.ConclusionEndServerTime > 0.0;

		if (bNewShowConclusionTimer)
		{
			NewRemainingSeconds = FMath::CeilToInt(GameState->GetRemainingConclusionTime());
		}
	}

	NewRemainingSeconds = FMath::Max(0, NewRemainingSeconds);

	if (UE_MVVM_SET_PROPERTY_VALUE(bShowConclusionTimer, bNewShowConclusionTimer))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetConclusionTimerVisibility);
	}

	if (UE_MVVM_SET_PROPERTY_VALUE(RemainingConclusionSeconds, NewRemainingSeconds))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetConclusionRemainingTimeText);
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
	RefreshConclusionTime();

	if (bPhaseChanged)
	{
		ShowPhaseNotification(NewMatchState.Phase);
	}
}

void UNF_GameFlowViewModel::HandleEscapeProgressChanged(const FNF_EscapeProgress& NewProgress)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(bShowEscapeProgress, NewProgress.bInProgress))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetEscapeProgressVisibility);
	}

	if (UE_MVVM_SET_PROPERTY_VALUE(bEscapePaused, NewProgress.bPaused))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetEscapePausedVisibility);
	}

	UE_MVVM_SET_PROPERTY_VALUE(EscapeState, NewProgress.State);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetEscapeStatusText);

	if (UE_MVVM_SET_PROPERTY_VALUE(EscapeTotalSeconds, FMath::Max(0.0f, NewProgress.TotalDuration)))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetEscapeTotalTimeText);
	}

	RefreshEscapeRemainingTime();
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

FText UNF_GameFlowViewModel::GetEscapeStatusText() const
{
	if (!IsValid(GameState))
	{
		return FText::GetEmpty();
	}

	const FNF_EscapeProgress Progress = GameState->GetEscapeProgress();
	if (!Progress.bInProgress)
	{
		return FText::GetEmpty();
	}

	const ANF_PlayerState* LocalPlayerState =
		Cast<ANF_PlayerState>(GetViewModelContext().PlayerState.Get());
	if (!IsValid(LocalPlayerState))
	{
		return FText::GetEmpty();
	}

	// 팀 ID는 이미 배정 및 복제가 완료된 상태를 전제로 한다.
	if (LocalPlayerState->NF_TeamID != Progress.TeamId)
	{
		return FText::FromString(TEXT("다른 팀이 탈출을 시도하고 있습니다."));
	}

	switch (Progress.State)
	{
	case ENF_EscapeState::CountingWithTeammate:
		return FText::FromString(TEXT("탈출 진행 중"));

	case ENF_EscapeState::PausedForTeammate:
		return FText::FromString(TEXT("팀원 대기"));

	case ENF_EscapeState::CountingSolo:
		return FText::FromString(TEXT("단독 탈출 진행 중"));

	case ENF_EscapeState::Idle:
	default:
		return FText::GetEmpty();
	}
}

FText UNF_GameFlowViewModel::GetEscapeRemainingTimeText() const
{
	return FText::FromString(FString::Printf(TEXT("%.1f초"), EscapeRemainingSeconds));
}

FText UNF_GameFlowViewModel::GetEscapeTotalTimeText() const
{
	return FText::FromString(FString::Printf(TEXT("%.1f초"), EscapeTotalSeconds));
}

float UNF_GameFlowViewModel::GetEscapeProgressPercent() const
{
	return EscapeProgressPercent;
}

ESlateVisibility UNF_GameFlowViewModel::GetEscapeProgressVisibility() const
{
	return bShowEscapeProgress ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
}

ESlateVisibility UNF_GameFlowViewModel::GetEscapePausedVisibility() const
{
	return bEscapePaused ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
}

FText UNF_GameFlowViewModel::GetConclusionRemainingTimeText() const
{
	return FText::FromString(
		FString::Printf(TEXT("서버 연결 종료까지 %d초"), RemainingConclusionSeconds));
}

ESlateVisibility UNF_GameFlowViewModel::GetConclusionTimerVisibility() const
{
	return bShowConclusionTimer ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
}
