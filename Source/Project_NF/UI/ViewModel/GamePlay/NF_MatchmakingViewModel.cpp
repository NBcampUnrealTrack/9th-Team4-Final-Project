// Fill out your copyright notice in the Description page of Project Settings.


#include "NF_MatchmakingViewModel.h"

#include "OnlineSubsystem.h"
#include "Core/Match/NF_MatchmakingSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"


void UNF_MatchmakingViewModel::OnInitializeViewModel()
{
	Super::OnInitializeViewModel();

	BindMatchmakingSubsystem();
	StartRefreshTimer();
	RefreshAll();
}

void UNF_MatchmakingViewModel::OnViewModelContextChanged()
{
	Super::OnViewModelContextChanged();

	UnbindMatchmakingSubsystem();
	StopRefreshTimer();

	BindMatchmakingSubsystem();
	StartRefreshTimer();
	RefreshAll();
}

void UNF_MatchmakingViewModel::OnDeinitializeViewModel()
{
	StopRefreshTimer();
	UnbindMatchmakingSubsystem();

	Super::OnDeinitializeViewModel();
}

void UNF_MatchmakingViewModel::BindMatchmakingSubsystem()
{
	ULocalPlayer* LocalPlayer =GetViewModelContext().LocalPlayer.Get();

	if (!IsValid(LocalPlayer)) return;

	UGameInstance* GameInstance =LocalPlayer->GetGameInstance();

	if (!IsValid(GameInstance)) return;
	
	MatchmakingSubsystem =GameInstance->GetSubsystem<UNF_MatchmakingSubsystem>();

	if (!IsValid(MatchmakingSubsystem))return;

	MatchmakingSubsystem->OnMatchStateChanged.AddUniqueDynamic(this,&ThisClass::HandleMatchStateChanged);

	MatchmakingSubsystem->OnQueueCountChanged.AddUniqueDynamic(this,&ThisClass::HandleQueueCountChanged);

	MatchmakingSubsystem->OnMatchmakingFailed.AddUniqueDynamic(this,&ThisClass::HandleMatchmakingFailed);
}

void UNF_MatchmakingViewModel::UnbindMatchmakingSubsystem()
{
	if (IsValid(MatchmakingSubsystem))
	{
		MatchmakingSubsystem->OnMatchStateChanged.RemoveAll(this);
		MatchmakingSubsystem->OnQueueCountChanged.RemoveAll(this);
		MatchmakingSubsystem->OnMatchmakingFailed.RemoveAll(this);
	}

	MatchmakingSubsystem = nullptr;
}

void UNF_MatchmakingViewModel::StartRefreshTimer()
{
	ULocalPlayer* LocalPlayer =GetViewModelContext().LocalPlayer.Get();

	if (!IsValid(LocalPlayer)) return;

	UWorld* World = LocalPlayer->GetWorld();

	if (!IsValid(World)) return;

	RefreshTimerWorld = World;

	World->GetTimerManager().SetTimer(
		RefreshTimerHandle,
		this,
		&ThisClass::RefreshElapsedTime,
		1.0f,
		true);
}

void UNF_MatchmakingViewModel::StopRefreshTimer()
{
	UWorld* World = RefreshTimerWorld.Get();

	if (IsValid(World))
	{
		World->GetTimerManager().ClearTimer(RefreshTimerHandle);
	}

	RefreshTimerHandle.Invalidate();
	RefreshTimerWorld.Reset();
}

void UNF_MatchmakingViewModel::RefreshAll()
{
	if (!IsValid(MatchmakingSubsystem))
	{
		UpdateMatchState(ENFMatchState::Ended);
		UpdateQueueCount(0,NF_MatchRules::PlayerCount);

		UE_MVVM_SET_PROPERTY_VALUE(ElapsedSeconds,0);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetElapsedTimeText);

		RefreshSteamStatus();
		return;
	}

	UpdateMatchState(MatchmakingSubsystem->GetMatchState());
	UpdateQueueCount(MatchmakingSubsystem->GetQueuedPlayerCount(),ResolveRequiredPlayerCount());

	RefreshElapsedTime();
	RefreshSteamStatus();
}

void UNF_MatchmakingViewModel::RefreshElapsedTime()
{
	int32 NewElapsedSeconds = 0;

	if (IsValid(MatchmakingSubsystem))
	{
		NewElapsedSeconds =MatchmakingSubsystem->GetQueueElapsedSeconds();
	}

	if (UE_MVVM_SET_PROPERTY_VALUE(ElapsedSeconds,NewElapsedSeconds))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetElapsedTimeText);
	}
}

void UNF_MatchmakingViewModel::RefreshSteamStatus()
{
	IOnlineSubsystem* SteamSubsystem =IOnlineSubsystem::Get(FName(TEXT("STEAM")));

	if (SteamSubsystem == nullptr)
	{
		UpdateSteamStatus(
			ENFSteamConnectionState::Unavailable,
			FText::FromString(
				TEXT("Steam 서비스를 사용할 수 없습니다.")));

		return;
	}

	const IOnlineIdentityPtr Identity =SteamSubsystem->GetIdentityInterface();

	if (!Identity.IsValid())
	{
		UpdateSteamStatus(
			ENFSteamConnectionState::IdentityUnavailable,
			FText::FromString(
				TEXT("Steam 사용자 정보를 가져올 수 없습니다.")));

		return;
	}

	const ELoginStatus::Type LoginStatus =Identity->GetLoginStatus(0);

	if (LoginStatus == ELoginStatus::UsingLocalProfile)
	{
		UpdateSteamStatus(
			ENFSteamConnectionState::Offline,
			FText::FromString(
				TEXT("Steam 오프라인 프로필 사용 중")));

		return;
	}

	if (LoginStatus != ELoginStatus::LoggedIn)
	{
		UpdateSteamStatus(
			ENFSteamConnectionState::Offline,
			FText::FromString(
				TEXT("Steam에 로그인되어 있지 않습니다.")));

		return;
	}

	const FUniqueNetIdPtr LocalUserId =Identity->GetUniquePlayerId(0);
	
	if (!LocalUserId.IsValid())
	{
		UpdateSteamStatus(
			ENFSteamConnectionState::MissingUserId,
			FText::FromString(
				TEXT("Steam 사용자 ID를 확인할 수 없습니다.")));

		return;
	}

	UpdateSteamStatus(
		ENFSteamConnectionState::Connected,
		FText::FromString(
			TEXT("Steam 연결됨")));
}

int32 UNF_MatchmakingViewModel::ResolveRequiredPlayerCount() const
{
	if (!IsValid(MatchmakingSubsystem))
	{
		return NF_MatchRules::PlayerCount;
	}

	const FName CurrentQueueId =MatchmakingSubsystem->GetCurrentQueueId();

	const TArray<FNFMatchQueueDefinition> QueueDefinitions =MatchmakingSubsystem->GetAvailableMatchQueues();

	for (const FNFMatchQueueDefinition& Definition: QueueDefinitions)
	{
		if (Definition.QueueId == CurrentQueueId)
		{
			return Definition.PlayerCount;
		}
	}

	return NF_MatchRules::PlayerCount;
}

void UNF_MatchmakingViewModel::UpdateMatchState(const ENFMatchState NewState)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(MatchState,NewState))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetMatchStateText);

		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetCanStartMatchmaking);

		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetCanCancelMatchmaking);
	}

	if (NewState == ENFMatchState::Queueing)
	{
		SetFailureMessage(FText::GetEmpty());
	}
}

void UNF_MatchmakingViewModel::UpdateQueueCount(const int32 NewQueuedPlayerCount,const int32 NewRequiredPlayerCount)
{
	bool bPlayerCountChanged = false;

	bPlayerCountChanged |= UE_MVVM_SET_PROPERTY_VALUE(QueuedPlayerCount,FMath::Max(0, NewQueuedPlayerCount));
	bPlayerCountChanged |= UE_MVVM_SET_PROPERTY_VALUE(RequiredPlayerCount,FMath::Max(1, NewRequiredPlayerCount));

	if (!bPlayerCountChanged) return;
	
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetPlayerCountText);
	
}

void UNF_MatchmakingViewModel::UpdateSteamStatus(const ENFSteamConnectionState NewState,const FText& NewStatusText)
{
	UE_MVVM_SET_PROPERTY_VALUE(SteamConnectionState,NewState);
	UE_MVVM_SET_PROPERTY_VALUE(SteamStatusText,NewStatusText);
}

void UNF_MatchmakingViewModel::SetFailureMessage(const FText& NewFailureMessage)
{
	UE_MVVM_SET_PROPERTY_VALUE(FailureMessage,NewFailureMessage);
}

void UNF_MatchmakingViewModel::HandleMatchStateChanged(const ENFMatchState NewState)
{
	UpdateMatchState(NewState);
	RefreshElapsedTime();
}

void UNF_MatchmakingViewModel::HandleQueueCountChanged(const int32 PlayerCount,const int32 RequiredPlayers)
{
	UpdateQueueCount(PlayerCount,RequiredPlayers);
}

void UNF_MatchmakingViewModel::HandleMatchmakingFailed(const FString& Reason)
{
	SetFailureMessage(FText::FromString(Reason));

	if (IsValid(MatchmakingSubsystem))
	{
		UpdateMatchState(MatchmakingSubsystem->GetMatchState());
	}

	RefreshElapsedTime();
}

bool UNF_MatchmakingViewModel::StartMatchmaking()
{
	if (!IsValid(MatchmakingSubsystem))
	{
		SetFailureMessage(FText::FromString(TEXT("매치메이킹 서비스를 사용할 수 없습니다.")));

		return false;
	}

	SetFailureMessage(FText::GetEmpty());

	const bool bStarted =MatchmakingSubsystem->StartLocalMatchmaking();

	if (!bStarted && FailureMessage.IsEmpty())
	{
		SetFailureMessage(FText::FromString(TEXT("매칭을 시작하지 못했습니다. Steam 상태를 확인하세요.")));
	}

	RefreshAll();

	return bStarted;
}

void UNF_MatchmakingViewModel::CancelMatchmaking()
{
	if (!IsValid(MatchmakingSubsystem))return;
	if (!GetCanCancelMatchmaking())return;
	
	MatchmakingSubsystem->CancelLocalMatchmaking();
	RefreshAll();
}

FText UNF_MatchmakingViewModel::GetMatchStateText() const
{
	switch (MatchState)
	{
	case ENFMatchState::Queueing:
		return FText::FromString(TEXT("매칭 요청 중"));

	case ENFMatchState::LobbyWaiting:
		return FText::FromString(TEXT("플레이어 대기 중"));

	case ENFMatchState::ServerPreparing:
		return FText::FromString(
			TEXT("팀 편성 완료 · 서버 준비 중"));

	case ENFMatchState::ServerConnecting:
		return FText::FromString(TEXT("서버 접속 중"));

	case ENFMatchState::InProgress:
		return FText::FromString(TEXT("게임 진행 중"));

	case ENFMatchState::Canceled:
		return FText::FromString(TEXT("매칭 취소됨"));

	case ENFMatchState::Ended:
	default:
		return FText::FromString(
			TEXT("대기열에 참가하지 않음"));
	}
}

FText UNF_MatchmakingViewModel::GetPlayerCountText() const
{
	return FText::FromString(FString::Printf(TEXT("인원 %d / %d"),QueuedPlayerCount,RequiredPlayerCount));
}

FText UNF_MatchmakingViewModel::GetElapsedTimeText() const
{
	const int32 Minutes = ElapsedSeconds / 60;
	const int32 Seconds = ElapsedSeconds % 60;

	return FText::FromString(FString::Printf(TEXT("대기 시간 %02d:%02d"),Minutes,Seconds));
}

bool UNF_MatchmakingViewModel::GetCanStartMatchmaking() const
{
	if (MatchState == ENFMatchState::Ended){return true;}
	if (MatchState == ENFMatchState::Canceled){return true;}

	return false;
}

bool UNF_MatchmakingViewModel::GetCanCancelMatchmaking() const
{
	if (MatchState == ENFMatchState::Queueing){return true;}
	if (MatchState == ENFMatchState::LobbyWaiting){return true;}

	return false;
}