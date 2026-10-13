// Copyright Epic Games, Inc. All Rights Reserved.

#include "NF_MatchmakingWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Core/Match/NF_MatchmakingSubsystem.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "OnlineSubsystem.h"
#include "Project_NF.h"
#include "steam/steam_api.h"
#include "TimerManager.h"

namespace
{
	UTextBlock* AddLabel(UWidgetTree* WidgetTree, UVerticalBox* Parent, const TCHAR* Text, int32 FontSize)
	{
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(FText::FromString(Text));
		Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.93f, 1.0f)));
		FSlateFontInfo Font = Label->GetFont();
		Font.Size = FontSize;
		Label->SetFont(Font);
		if (UVerticalBoxSlot* Slot = Parent->AddChildToVerticalBox(Label))
		{
			Slot->SetPadding(FMargin(0.0f, 5.0f));
		}
		return Label;
	}

	void SetButtonText(UWidgetTree* WidgetTree, UButton* Button, const TCHAR* Text)
	{
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(FText::FromString(Text));
		Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		FSlateFontInfo Font = Label->GetFont();
		Font.Size = 18;
		Label->SetFont(Font);
		Button->AddChild(Label);
	}
}

void UNF_MatchmakingWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildWidgetTree();
	if (StartButton)
	{
		StartButton->OnClicked.AddDynamic(this, &ThisClass::HandleStartButtonClicked);
	}
	if (CancelButton)
	{
		CancelButton->OnClicked.AddDynamic(this, &ThisClass::HandleCancelButtonClicked);
	}
}

void UNF_MatchmakingWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		MatchmakingSubsystem = GameInstance->GetSubsystem<UNF_MatchmakingSubsystem>();
	}
	if (MatchmakingSubsystem)
	{
		MatchmakingSubsystem->OnMatchStateChanged.AddDynamic(this, &ThisClass::HandleMatchStateChanged);
		MatchmakingSubsystem->OnQueueCountChanged.AddDynamic(this, &ThisClass::HandleQueueCountChanged);
		MatchmakingSubsystem->OnMatchmakingFailed.AddDynamic(this, &ThisClass::HandleMatchmakingFailed);
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(DisplayRefreshTimerHandle, this, &ThisClass::RefreshDisplay, 1.0f, true);
	}
	RefreshDisplay();
}

void UNF_MatchmakingWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DisplayRefreshTimerHandle);
	}
	if (MatchmakingSubsystem)
	{
		MatchmakingSubsystem->OnMatchStateChanged.RemoveAll(this);
		MatchmakingSubsystem->OnQueueCountChanged.RemoveAll(this);
		MatchmakingSubsystem->OnMatchmakingFailed.RemoveAll(this);
		MatchmakingSubsystem = nullptr;
	}
	Super::NativeDestruct();
}

void UNF_MatchmakingWidget::BuildWidgetTree()
{
	if (!WidgetTree)
	{
		return;
	}

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RootOverlay"));
	WidgetTree->RootWidget = Root;
	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MatchmakingPanelSize"));
	PanelSize->SetWidthOverride(440.0f);
	PanelSize->SetMinDesiredHeight(300.0f);
	UOverlaySlot* CenterSlot = Root->AddChildToOverlay(PanelSize);
	CenterSlot->SetHorizontalAlignment(HAlign_Center);
	CenterSlot->SetVerticalAlignment(VAlign_Center);

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MatchmakingPanel"));
	Panel->SetPadding(FMargin(24.0f));
	Panel->SetBrushColor(FLinearColor(0.025f, 0.045f, 0.08f, 0.94f));
	PanelSize->SetContent(Panel);

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MatchmakingContent"));
	Panel->SetContent(Content);
	AddLabel(WidgetTree, Content, TEXT("STEAM MATCHMAKING"), 22);
	SteamStatusText = AddLabel(WidgetTree, Content, TEXT("Steam 상태 확인 중..."), 14);
	StateText = AddLabel(WidgetTree, Content, TEXT("대기열에 참가하지 않음"), 16);
	PlayerCountText = AddLabel(WidgetTree, Content, TEXT("인원 0 / 8"), 28);
	ElapsedTimeText = AddLabel(WidgetTree, Content, TEXT("대기 시간 00:00"), 18);
	FeedbackText = AddLabel(WidgetTree, Content, TEXT(""), 13);

	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MatchmakingButtons"));
	Content->AddChildToVerticalBox(Buttons);
	StartButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("StartMatchmakingButton"));
	StartButton->SetBackgroundColor(FLinearColor(0.08f, 0.38f, 0.75f, 1.0f));
	SetButtonText(WidgetTree, StartButton, TEXT("매칭 시작"));
	if (UHorizontalBoxSlot* ButtonSlot = Buttons->AddChildToHorizontalBox(StartButton))
	{
		ButtonSlot->SetPadding(FMargin(0.0f, 8.0f, 6.0f, 0.0f));
		ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	CancelButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CancelMatchmakingButton"));
	CancelButton->SetBackgroundColor(FLinearColor(0.55f, 0.12f, 0.12f, 1.0f));
	SetButtonText(WidgetTree, CancelButton, TEXT("매칭 취소"));
	if (UHorizontalBoxSlot* ButtonSlot = Buttons->AddChildToHorizontalBox(CancelButton))
	{
		ButtonSlot->SetPadding(FMargin(6.0f, 8.0f, 0.0f, 0.0f));
		ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
}

void UNF_MatchmakingWidget::RefreshDisplay()
{
	RefreshSteamStatus();
	if (!MatchmakingSubsystem)
	{
		if (StateText) StateText->SetText(FText::FromString(TEXT("매치메이킹 서비스를 사용할 수 없음")));
		if (PlayerCountText) PlayerCountText->SetText(FText::FromString(TEXT("인원 0 / 8")));
		if (ElapsedTimeText) ElapsedTimeText->SetText(FText::FromString(TEXT("대기 시간 00:00")));
		if (StartButton) StartButton->SetVisibility(ESlateVisibility::Visible);
		if (CancelButton) CancelButton->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const ENFMatchState State = MatchmakingSubsystem->GetMatchState();
	const bool bQueueActive = State == ENFMatchState::Queueing || State == ENFMatchState::LobbyWaiting;
	const int32 PlayerCount = MatchmakingSubsystem->GetQueuedPlayerCount();
	const int32 Elapsed = MatchmakingSubsystem->GetQueueElapsedSeconds();
	if (PlayerCountText)
	{
		PlayerCountText->SetText(FText::FromString(FString::Printf(TEXT("인원 %d / %d"), PlayerCount, NF_MatchRules::PlayerCount)));
	}
	if (ElapsedTimeText)
	{
		ElapsedTimeText->SetText(FText::FromString(FString::Printf(TEXT("대기 시간 %02d:%02d"), Elapsed / 60, Elapsed % 60)));
	}
	if (StartButton) StartButton->SetVisibility(bQueueActive || State == ENFMatchState::ServerPreparing ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	if (CancelButton) CancelButton->SetVisibility(bQueueActive ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (StateText)
	{
		FString StateLabel;
		switch (State)
		{
		case ENFMatchState::Queueing: StateLabel = TEXT("매칭 요청 중"); break;
		case ENFMatchState::LobbyWaiting: StateLabel = TEXT("플레이어 대기 중"); break;
		case ENFMatchState::ServerPreparing: StateLabel = TEXT("팀 편성 완료 · 서버 준비 중"); break;
		case ENFMatchState::ServerConnecting: StateLabel = TEXT("서버 접속 중"); break;
		case ENFMatchState::InProgress: StateLabel = TEXT("게임 진행 중"); break;
		case ENFMatchState::Canceled: StateLabel = TEXT("매칭 취소됨"); break;
		default: StateLabel = TEXT("대기열에 참가하지 않음"); break;
		}
		StateText->SetText(FText::FromString(StateLabel));
	}
}

void UNF_MatchmakingWidget::RefreshSteamStatus()
{
	if (!SteamStatusText)
	{
		return;
	}

	IOnlineSubsystem* DefaultSubsystem = IOnlineSubsystem::Get();
	const FString DefaultName = DefaultSubsystem ? DefaultSubsystem->GetSubsystemName().ToString() : TEXT("none");
	LogSteamStartupDiagnostics(DefaultName);
	IOnlineSubsystem* SteamSubsystem = IOnlineSubsystem::Get(FName(TEXT("STEAM")));
	if (!SteamSubsystem)
	{
		SetSteamStatus(FString::Printf(TEXT("Steam OSS: 사용할 수 없음 (default OSS: %s)"), *DefaultName), FLinearColor(1.0f, 0.25f, 0.25f), true);
		return;
	}

	const IOnlineIdentityPtr Identity = SteamSubsystem->GetIdentityInterface();
	if (!Identity.IsValid())
	{
		SetSteamStatus(TEXT("Steam OSS 인스턴스는 있지만 Identity 인터페이스를 가져오지 못했습니다."), FLinearColor(1.0f, 0.7f, 0.2f), true);
		return;
	}

	const ELoginStatus::Type LoginStatus = Identity->GetLoginStatus(0);
	const FUniqueNetIdPtr LocalUserId = Identity->GetUniquePlayerId(0);
	if (LoginStatus == ELoginStatus::LoggedIn && LocalUserId.IsValid())
	{
		SetSteamStatus(TEXT("Steam OSS 연결 및 로컬 사용자 로그인 성공"), FLinearColor(0.25f, 0.9f, 0.4f), false);
	}
	else if (LoginStatus == ELoginStatus::UsingLocalProfile)
	{
		SetSteamStatus(TEXT("Steam OSS가 로컬 오프라인 프로필을 사용 중입니다."), FLinearColor(1.0f, 0.7f, 0.2f), true);
	}
	else if (LoginStatus == ELoginStatus::LoggedIn)
	{
		SetSteamStatus(TEXT("Steam 로그인 상태지만 로컬 사용자 Steam ID가 없습니다."), FLinearColor(1.0f, 0.45f, 0.25f), true);
	}
	else
	{
		SetSteamStatus(TEXT("Steam OSS가 로드됐지만 로컬 사용자 로그인 상태가 아닙니다."), FLinearColor(1.0f, 0.45f, 0.25f), true);
	}
}

void UNF_MatchmakingWidget::LogSteamStartupDiagnostics(const FString& DefaultSubsystemName)
{
	if (bLoggedSteamStartupDiagnostics)
	{
		return;
	}

	FString DefaultPlatformService = TEXT("<unset>");
	GConfig->GetString(TEXT("OnlineSubsystem"), TEXT("DefaultPlatformService"), DefaultPlatformService, GEngineIni);
	int32 ConfiguredAppId = 0;
	const bool bHasConfiguredAppId = GConfig->GetInt(TEXT("OnlineSubsystemSteam"), TEXT("SteamDevAppId"), ConfiguredAppId, GEngineIni);
	bool bSteamEnabled = false;
	GConfig->GetBool(TEXT("OnlineSubsystemSteam"), TEXT("bEnabled"), bSteamEnabled, GEngineIni);
	bool bInitServerOnClient = false;
	GConfig->GetBool(TEXT("OnlineSubsystemSteam"), TEXT("bInitServerOnClient"), bInitServerOnClient, GEngineIni);

	const bool bSteamClientRunning = SteamAPI_IsSteamRunning();
	const HSteamUser SteamUserHandle = SteamAPI_GetHSteamUser();
	const bool bSteamApiInitialized = SteamUserHandle != 0;
	FString RuntimeAppId = TEXT("unavailable (Steam API not initialized)");
	FString SteamLoggedOn = TEXT("unavailable (Steam API not initialized)");
	if (bSteamApiInitialized)
	{
		if (ISteamUtils* SteamUtilsInterface = SteamUtils())
		{
			RuntimeAppId = LexToString(SteamUtilsInterface->GetAppID());
		}
		if (ISteamUser* SteamUserInterface = SteamUser())
		{
			SteamLoggedOn = SteamUserInterface->BLoggedOn() ? TEXT("yes") : TEXT("no");
		}
	}

	const FString SteamAppIdFilePath = FPaths::Combine(FPlatformProcess::BaseDir(), TEXT("steam_appid.txt"));
	FString SteamAppIdFileContents = TEXT("not found");
	FFileHelper::LoadFileToString(SteamAppIdFileContents, *SteamAppIdFilePath);
	SteamAppIdFileContents.TrimStartAndEndInline();
	if (SteamAppIdFileContents.IsEmpty())
	{
		SteamAppIdFileContents = TEXT("empty or unreadable");
	}
	FString AppIdCheck = TEXT("cannot verify because Steam API is not initialized");
	if (bSteamApiInitialized && bHasConfiguredAppId)
	{
		AppIdCheck = FCString::Atoi(*RuntimeAppId) == ConfiguredAppId ? TEXT("match") : TEXT("MISMATCH");
	}

	UE_LOG(LogProject_NF, Warning,
		TEXT("Steam startup diagnosis: DefaultPlatformService=%s, Steam bEnabled=%s, configured SteamDevAppId=%s, AppID check=%s, bInitServerOnClient=%s, Steam client running=%s, Steam API initialized=%s, runtime AppID=%s, Steam user logged on=%s, steam_appid.txt path=%s value=%s, default OSS=%s"),
		*DefaultPlatformService,
		bSteamEnabled ? TEXT("true") : TEXT("false"),
		bHasConfiguredAppId ? *LexToString(ConfiguredAppId) : TEXT("missing"),
		*AppIdCheck,
		bInitServerOnClient ? TEXT("true") : TEXT("false"),
		bSteamClientRunning ? TEXT("yes") : TEXT("no"),
		bSteamApiInitialized ? TEXT("yes") : TEXT("no"),
		*RuntimeAppId,
		*SteamLoggedOn,
		*SteamAppIdFilePath,
		*SteamAppIdFileContents,
		*DefaultSubsystemName);

	bLoggedSteamStartupDiagnostics = true;
}

void UNF_MatchmakingWidget::SetSteamStatus(const FString& StatusText, const FLinearColor& Color, bool bIsError)
{
	SteamStatusText->SetText(FText::FromString(StatusText));
	SteamStatusText->SetColorAndOpacity(FSlateColor(Color));
	if (LastLoggedSteamStatus != StatusText)
	{
		if (bIsError)
		{
			UE_LOG(LogProject_NF, Warning, TEXT("Steam matchmaking UI diagnosis: %s"), *StatusText);
		}
		else
		{
			UE_LOG(LogProject_NF, Display, TEXT("Steam matchmaking UI diagnosis: %s"), *StatusText);
		}
		LastLoggedSteamStatus = StatusText;
	}
}

void UNF_MatchmakingWidget::HandleMatchStateChanged(ENFMatchState NewState)
{
	RefreshDisplay();
	if (FeedbackText && NewState == ENFMatchState::Queueing)
	{
		FeedbackText->SetText(FText::GetEmpty());
	}
}

void UNF_MatchmakingWidget::HandleQueueCountChanged(int32 PlayerCount, int32 RequiredPlayers)
{
	if (PlayerCountText)
	{
		PlayerCountText->SetText(FText::FromString(FString::Printf(TEXT("인원 %d / %d"), PlayerCount, RequiredPlayers)));
	}
}

void UNF_MatchmakingWidget::HandleMatchmakingFailed(const FString& Reason)
{
	if (FeedbackText)
	{
		FeedbackText->SetText(FText::FromString(Reason));
	}
	RefreshDisplay();
}

void UNF_MatchmakingWidget::HandleStartButtonClicked()
{
	if (!MatchmakingSubsystem || !MatchmakingSubsystem->StartLocalMatchmaking())
	{
		if (FeedbackText)
		{
			FeedbackText->SetText(FText::FromString(TEXT("매칭을 시작하지 못했습니다. Steam 로그인 및 현재 상태를 확인하세요.")));
		}
	}
	else if (FeedbackText)
	{
		FeedbackText->SetText(FText::GetEmpty());
	}
	RefreshDisplay();
}

void UNF_MatchmakingWidget::HandleCancelButtonClicked()
{
	if (MatchmakingSubsystem)
	{
		MatchmakingSubsystem->CancelLocalMatchmaking();
	}
	RefreshDisplay();
}
