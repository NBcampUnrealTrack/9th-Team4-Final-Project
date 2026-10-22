// Copyright Epic Games, Inc. All Rights Reserved.

#include "NF_MatchLobbyPlayerController.h"

#include "Core/Match/NF_DedicatedMatchSubsystem.h"
#include "Core/Match/NF_MatchmakingSubsystem.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "UI/SubSystem/NF_UIManagerSubsystem.h"
#include "UI/Widget/NF_MatchmakingWidget.h"
#include "Project_NF.h"
#include "Widgets/Input/SVirtualJoystick.h"

void ANF_MatchLobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SubmitPendingMatchTeamsToServer();

	if (ShouldUseTouchControls() && IsLocalPlayerController())
	{
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);
		if (MobileControlsWidget)
		{
			MobileControlsWidget->AddToPlayerScreen(0);
		}
		else
		{
			UE_LOG(LogProject_NF, Error, TEXT("Could not spawn mobile controls widget."));
		}
	}

	if (IsLocalPlayerController())
	{
		ULocalPlayer* LocalPlayer = GetLocalPlayer();
		UNF_UIManagerSubsystem* UIManager = IsValid(LocalPlayer)
			? LocalPlayer->GetSubsystem<UNF_UIManagerSubsystem>() : nullptr;
		if (IsValid(UIManager))
		{
			MatchmakingWidget = Cast<UNF_MatchmakingWidget>(UIManager->PushWidget(
				UNF_MatchmakingWidget::StaticClass(), ENF_UIInputMode::GameAndUI, true, 10));
		}
	}
}

void ANF_MatchLobbyPlayerController::PostNetInit()
{
	Super::PostNetInit();
	SubmitPendingMatchTeamsToServer();
}

bool ANF_MatchLobbyPlayerController::SubmitPendingMatchTeamsToServer()
{
	if (!IsLocalController() || !GetWorld() || GetWorld()->GetNetMode() != NM_Client)
	{
		return false;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UNF_MatchmakingSubsystem* MatchmakingSubsystem = GameInstance
		? GameInstance->GetSubsystem<UNF_MatchmakingSubsystem>() : nullptr;
	TArray<FNFMatchTeamAssignment> Teams;
	const FString LocalPlayerId = MatchmakingSubsystem ? MatchmakingSubsystem->GetLocalMatchPlayerId() : FString();
	if (!MatchmakingSubsystem || !MatchmakingSubsystem->HasDedicatedServerAllocation()
		|| LocalPlayerId.IsEmpty() || !MatchmakingSubsystem->GetPendingMatchTeams(Teams))
	{
		return false;
	}

	ServerSubmitMatchTeams(LocalPlayerId, Teams);
	return true;
}

void ANF_MatchLobbyPlayerController::ServerSubmitMatchTeams_Implementation(
	const FString& ClaimedPlayerId, const TArray<FNFMatchTeamAssignment>& Teams)
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UNF_DedicatedMatchSubsystem* MatchSubsystem = GameInstance->GetSubsystem<UNF_DedicatedMatchSubsystem>())
		{
			MatchSubsystem->SubmitTeamAssignments(this, ClaimedPlayerId, Teams);
		}
	}
}

void ANF_MatchLobbyPlayerController::ClientHandleMatchAborted_Implementation(const FString& Reason)
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UNF_MatchmakingSubsystem* MatchmakingSubsystem = GameInstance->GetSubsystem<UNF_MatchmakingSubsystem>())
		{
			MatchmakingSubsystem->HandleDedicatedMatchAborted(Reason);
		}
	}
}

void ANF_MatchLobbyPlayerController::ClientHandleMatchCompleted_Implementation(const FNF_MatchResult& Result)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UNF_MatchmakingSubsystem* Match = GI->GetSubsystem<UNF_MatchmakingSubsystem>())
		{
			Match->HandleDedicatedMatchCompleted(Result);
		}
	}
}

void ANF_MatchLobbyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!IsLocalPlayerController())
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
		{
			Subsystem->AddMappingContext(CurrentContext, 0);
		}

		if (!ShouldUseTouchControls())
		{
			for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}
		}
	}
}

bool ANF_MatchLobbyPlayerController::ShouldUseTouchControls() const
{
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
