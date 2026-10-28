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
#include "UI/ViewModel/GamePlay/NF_MatchmakingViewModel.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "UI/SubSystem/NF_ViewModelSubsystem.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "OnlineSubsystem.h"
#include "Project_NF.h"
#include "steam/steam_api.h"
#include "Engine/LocalPlayer.h"
#include "UObject/UnrealType.h"

namespace
{
	FText ReadViewModelText(const UNF_MatchmakingViewModel* ViewModel, FName FieldName)
	{
		const FTextProperty* Property = FindFProperty<FTextProperty>(ViewModel->GetClass(), FieldName);
		if (!ensureMsgf(Property, TEXT("Missing matchmaking text field: %s"), *FieldName.ToString()))
		{
			return FText::GetEmpty();
		}
		return Property->GetPropertyValue_InContainer(ViewModel);
	}

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
	DisconnectViewModel();
	if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
	{
		ViewModelSubsystem = LocalPlayer->GetSubsystem<UNF_ViewModelSubsystem>();
		if (UNF_ViewModelSubsystem* Subsystem = ViewModelSubsystem.Get())
		{
			MatchmakingViewModel = NewObject<UNF_MatchmakingViewModel>(this);
			// Native UMG uses the same FieldNotify signals as WBP view bindings.
			MatchmakingViewModel->GetFieldNotificationDescriptor().ForEachField(
				MatchmakingViewModel->GetClass(), [this](UE::FieldNotification::FFieldId Field)
				{
					MatchmakingViewModel->AddFieldValueChangedDelegate(Field,
						INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateWeakLambda(
							this, [this](UObject*, UE::FieldNotification::FFieldId)
							{
								RefreshDisplay();
							}));
					return true;
				});
			Subsystem->RegisterViewModel(MatchmakingViewModel);
		}
	}
	IOnlineSubsystem* DefaultSubsystem = IOnlineSubsystem::Get();
	LogSteamStartupDiagnostics(DefaultSubsystem ? DefaultSubsystem->GetSubsystemName().ToString() : TEXT("none"));
	// Initialization may retain default values without emitting FieldNotify.
	RefreshDisplay();
}

void UNF_MatchmakingWidget::DisconnectViewModel()
{
	if (MatchmakingViewModel)
	{
		MatchmakingViewModel->RemoveAllFieldValueChangedDelegates(this);
		if (UNF_ViewModelSubsystem* Subsystem = ViewModelSubsystem.Get())
		{
			Subsystem->UnregisterViewModel(MatchmakingViewModel);
		}
		else
		{
			MatchmakingViewModel->DeinitializeViewModel();
		}
		MatchmakingViewModel = nullptr;
	}
	ViewModelSubsystem.Reset();
}

void UNF_MatchmakingWidget::NativeDestruct()
{
	DisconnectViewModel();
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
	const bool bReady = IsValid(MatchmakingViewModel) && MatchmakingViewModel->IsViewModelInitialized();
	if (StateText) StateText->SetText(bReady ? MatchmakingViewModel->GetMatchStateText()
		: FText::FromString(TEXT("매치메이킹 서비스를 사용할 수 없음")));
	if (PlayerCountText) PlayerCountText->SetText(bReady ? MatchmakingViewModel->GetPlayerCountText() : FText::GetEmpty());
	if (ElapsedTimeText) ElapsedTimeText->SetText(bReady ? MatchmakingViewModel->GetElapsedTimeText() : FText::GetEmpty());
	if (StartButton)
	{
		const bool bCanStart = bReady && MatchmakingViewModel->GetCanStartMatchmaking();
		StartButton->SetVisibility(!bReady || bCanStart ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		StartButton->SetIsEnabled(bCanStart);
	}
	if (CancelButton)
	{
		const bool bCanCancel = bReady && MatchmakingViewModel->GetCanCancelMatchmaking();
		CancelButton->SetVisibility(bCanCancel ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		CancelButton->SetIsEnabled(bCanCancel);
	}
	if (FeedbackText)
	{
		FeedbackText->SetText(bReady ? ReadViewModelText(MatchmakingViewModel, TEXT("FailureMessage")) : FText::GetEmpty());
	}
	RefreshSteamStatus();
}

void UNF_MatchmakingWidget::RefreshSteamStatus()
{
	if (!SteamStatusText) return;
	if (!IsValid(MatchmakingViewModel))
	{
		SteamStatusText->SetText(FText::GetEmpty());
		return;
	}

	// These BlueprintReadOnly fields have no public C++ getters. Read the reflected
	// display contract without changing the existing ViewModel or querying its model.
	const FEnumProperty* Property = FindFProperty<FEnumProperty>(
		MatchmakingViewModel->GetClass(), TEXT("SteamConnectionState"));
	ENFSteamConnectionState State = ENFSteamConnectionState::Unavailable;
	if (ensureMsgf(Property, TEXT("Matchmaking ViewModel requires SteamConnectionState")))
	{
		State = static_cast<ENFSteamConnectionState>(Property->GetUnderlyingProperty()->GetSignedIntPropertyValue(
			Property->ContainerPtrToValuePtr<void>(MatchmakingViewModel.Get())));
	}
	const bool bConnected = State == ENFSteamConnectionState::Connected;
	const FLinearColor Color = bConnected ? FLinearColor(0.25f, 0.9f, 0.4f)
		: (State == ENFSteamConnectionState::Unavailable ? FLinearColor(1.0f, 0.25f, 0.25f)
			: FLinearColor(1.0f, 0.7f, 0.2f));
	SetSteamStatus(ReadViewModelText(MatchmakingViewModel, TEXT("SteamStatusText")).ToString(), Color, !bConnected);
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

void UNF_MatchmakingWidget::HandleStartButtonClicked()
{
	if (IsValid(MatchmakingViewModel))
	{
		MatchmakingViewModel->StartMatchmaking();
	}
}

void UNF_MatchmakingWidget::HandleCancelButtonClicked()
{
	if (IsValid(MatchmakingViewModel))
	{
		MatchmakingViewModel->CancelMatchmaking();
	}
}
