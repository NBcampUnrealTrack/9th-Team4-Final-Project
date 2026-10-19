#include "Core/Match/NF_DedicatedMatchSubsystem.h"

#include "Project_NF.h"
#include "Core/NF_GameMode.h"
#include "Core/NF_GameState.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformMisc.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Core/Match/Lobby/NF_MatchLobbyPlayerController.h"
#include "Player/State/NF_PlayerState.h"
#include "TimerManager.h"

namespace
{
	constexpr int32 ExpectedPlayerCount = 8;
	constexpr int32 ExpectedTeamCount = 4;
	constexpr int32 PlayersPerTeam = 2;
	constexpr float ServerExitDelaySeconds = 3.0f;
	const FName MatchSessionName = NAME_GameSession;
}

void UNF_DedicatedMatchSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DedicatedServerExitTimerHandle);
		if (DestroySessionCompleteHandle.IsValid())
		{
			if (IOnlineSubsystem* OSS = Online::GetSubsystem(World))
			{
				if (IOnlineSessionPtr SessionInterface = OSS->GetSessionInterface(); SessionInterface.IsValid())
				{
					SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
				}
			}
		}
	}
	Super::Deinitialize();
}

bool UNF_DedicatedMatchSubsystem::SubmitTeamAssignments(
	APlayerController* Submitter, const FString& ClaimedPlayerId,
	const TArray<FNFMatchTeamAssignment>& Teams)
{
	UWorld* World = GetWorld();
	ANF_GameMode* GameMode = World ? World->GetAuthGameMode<ANF_GameMode>() : nullptr;
	ANF_GameState* GameState = World ? World->GetGameState<ANF_GameState>() : nullptr;
	if (!World || !GameMode || !GameState || !IsValid(Submitter) || !Submitter->HasAuthority()
		|| GameState->GetMatchPhase() != ENF_MatchPhase::Waiting || bTeamAssignmentsApplied)
	{
		return false;
	}

	FString ValidationError;
	if (!ValidateTeamAssignments(Teams, ValidationError))
	{
		UE_LOG(LogProject_NF, Warning, TEXT("Rejected lobby team assignment: %s"), *ValidationError);
		return false;
	}

	ANF_PlayerState* SubmitterState = Submitter->GetPlayerState<ANF_PlayerState>();
	if (!IsValid(SubmitterState) || ClaimedPlayerId.IsEmpty())
	{
		return false;
	}

	const FUniqueNetIdRepl& OnlineId = SubmitterState->GetUniqueId();
	if (OnlineId.IsValid() && OnlineId.ToString() != ClaimedPlayerId)
	{
		UE_LOG(LogProject_NF, Warning, TEXT("Rejected claimed lobby ID %s; authenticated ID is %s."),
			*ClaimedPlayerId, *OnlineId.ToString());
		return false;
	}
	if (!OnlineId.IsValid())
	{
		UE_LOG(LogProject_NF, Warning,
			TEXT("No authenticated server ID for this connection; accepting claimed ID %s for this playtest."), *ClaimedPlayerId);
	}

	bool bSubmitterInRoster = false;
	for (const FNFMatchTeamAssignment& Team : Teams)
	{
		for (const FNFQueuedPlayer& Player : Team.Players)
		{
			bSubmitterInRoster |= Player.PlayerId == ClaimedPlayerId;
		}
	}
	if (!bSubmitterInRoster)
	{
		return false;
	}

	const FString Signature = MakeAssignmentSignature(Teams);
	if (ExpectedAssignmentSignature.IsEmpty())
	{
		ExpectedAssignmentSignature = Signature;
		CurrentTeams = Teams;
	}
	else if (ExpectedAssignmentSignature != Signature)
	{
		UE_LOG(LogProject_NF, Error, TEXT("Rejected inconsistent team assignment from %s."), *ClaimedPlayerId);
		return false;
	}

	if (const TWeakObjectPtr<ANF_PlayerState>* ExistingState = SubmittedPlayerStates.Find(ClaimedPlayerId))
	{
		if (ExistingState->Get() != SubmitterState)
		{
			return false;
		}
	}

	SubmittedPlayerIds.Add(ClaimedPlayerId);
	SubmittedPlayerStates.Add(ClaimedPlayerId, SubmitterState);
	TryApplyTeamAssignments();
	return true;
}

bool UNF_DedicatedMatchSubsystem::ValidateTeamAssignments(
	const TArray<FNFMatchTeamAssignment>& Teams, FString& OutError) const
{
	if (Teams.Num() != ExpectedTeamCount)
	{
		OutError = FString::Printf(TEXT("expected %d teams, received %d"), ExpectedTeamCount, Teams.Num());
		return false;
	}

	TSet<FString> PlayerIds;
	TMap<FString, TArray<FNFQueuedPlayer>> Parties;
	TMap<FString, int32> PartyTeams;
	for (int32 TeamIndex = 0; TeamIndex < Teams.Num(); ++TeamIndex)
	{
		const FNFMatchTeamAssignment& Team = Teams[TeamIndex];
		if (Team.Players.Num() != PlayersPerTeam)
		{
			OutError = FString::Printf(TEXT("team %d must have exactly %d players"), TeamIndex + 1, PlayersPerTeam);
			return false;
		}

		for (const FNFQueuedPlayer& Player : Team.Players)
		{
			if (Player.PlayerId.IsEmpty() || Player.PartyId.IsEmpty() || Player.PartyLeaderId.IsEmpty()
				|| PlayerIds.Contains(Player.PlayerId))
			{
				OutError = TEXT("roster contains an empty identity or duplicate player");
				return false;
			}
			PlayerIds.Add(Player.PlayerId);
			Parties.FindOrAdd(Player.PartyId).Add(Player);

			if (const int32* ExistingTeam = PartyTeams.Find(Player.PartyId))
			{
				if (*ExistingTeam != TeamIndex)
				{
					OutError = FString::Printf(TEXT("party %s is split across teams"), *Player.PartyId);
					return false;
				}
			}
			else
			{
				PartyTeams.Add(Player.PartyId, TeamIndex);
			}
		}
	}

	if (PlayerIds.Num() != ExpectedPlayerCount)
	{
		OutError = FString::Printf(TEXT("expected %d unique players, received %d"), ExpectedPlayerCount, PlayerIds.Num());
		return false;
	}
	for (const TPair<FString, TArray<FNFQueuedPlayer>>& Party : Parties)
	{
		if (Party.Value.Num() > 2)
		{
			OutError = FString::Printf(TEXT("party %s exceeds the two-player limit"), *Party.Key);
			return false;
		}
		const FString& LeaderId = Party.Value[0].PartyLeaderId;
		bool bLeaderIsMember = false;
		for (const FNFQueuedPlayer& Player : Party.Value)
		{
			if (Player.PartyLeaderId != LeaderId)
			{
				OutError = FString::Printf(TEXT("party %s has inconsistent leader IDs"), *Party.Key);
				return false;
			}
			bLeaderIsMember |= Player.PlayerId == LeaderId;
		}
		if (!bLeaderIsMember)
		{
			OutError = FString::Printf(TEXT("party %s leader is not in the roster"), *Party.Key);
			return false;
		}
	}
	return true;
}

FString UNF_DedicatedMatchSubsystem::MakeAssignmentSignature(const TArray<FNFMatchTeamAssignment>& Teams) const
{
	TArray<FString> SerializedTeams;
	for (const FNFMatchTeamAssignment& Team : Teams)
	{
		TArray<FString> Members;
		for (const FNFQueuedPlayer& Player : Team.Players)
		{
			Members.Add(FString::Printf(TEXT("%s,%s,%s"), *Player.PlayerId, *Player.PartyId, *Player.PartyLeaderId));
		}
		Members.Sort();
		SerializedTeams.Add(FString::Join(Members, TEXT(";")));
	}
	return FString::Join(SerializedTeams, TEXT("|"));
}

void UNF_DedicatedMatchSubsystem::TryApplyTeamAssignments()
{
	if (bTeamAssignmentsApplied || SubmittedPlayerIds.Num() != ExpectedPlayerCount)
	{
		return;
	}

	UWorld* World = GetWorld();
	ANF_GameMode* GameMode = World ? World->GetAuthGameMode<ANF_GameMode>() : nullptr;
	ANF_GameState* GameState = World ? World->GetGameState<ANF_GameState>() : nullptr;
	if (!World || !GameMode || !GameState || GameMode->GetNumPlayers() != ExpectedPlayerCount)
	{
		return;
	}

	TSet<ANF_PlayerState*> ConnectedStates;
	for (APlayerState* BaseState : GameState->PlayerArray)
	{
		ANF_PlayerState* PlayerState = Cast<ANF_PlayerState>(BaseState);
		if (!IsValid(PlayerState))
		{
			UE_LOG(LogProject_NF, Error, TEXT("Cannot apply teams: a connection does not use ANF_PlayerState."));
			return;
		}
		ConnectedStates.Add(PlayerState);
	}
	if (ConnectedStates.Num() != ExpectedPlayerCount)
	{
		return;
	}

	for (const FNFMatchTeamAssignment& Team : CurrentTeams)
	{
		for (const FNFQueuedPlayer& Player : Team.Players)
		{
			const TWeakObjectPtr<ANF_PlayerState>* State = SubmittedPlayerStates.Find(Player.PlayerId);
			if (!State || !State->IsValid() || !ConnectedStates.Contains(State->Get()))
			{
				UE_LOG(LogProject_NF, Error, TEXT("Lobby player %s is not connected to this server."), *Player.PlayerId);
				return;
			}
		}
	}

	for (int32 TeamIndex = 0; TeamIndex < CurrentTeams.Num(); ++TeamIndex)
	{
		for (const FNFQueuedPlayer& Player : CurrentTeams[TeamIndex].Players)
		{
			const TWeakObjectPtr<ANF_PlayerState>* State = SubmittedPlayerStates.Find(Player.PlayerId);
			if (!State || !State->IsValid() || !State->Get()->SetNFTeamID(TeamIndex + 1))
			{
				UE_LOG(LogProject_NF, Error, TEXT("Failed to set team %d for %s."), TeamIndex + 1, *Player.PlayerId);
				return;
			}
		}
	}

	bTeamAssignmentsApplied = true;
	GameMode->NotifyTeamsReady();
	UE_LOG(LogProject_NF, Display, TEXT("Applied lobby teams to all %d players."), ExpectedPlayerCount);
}

void UNF_DedicatedMatchSubsystem::HandleMatchAbort(const FString& Reason)
{
	if (bMatchAbortHandled)
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client)
	{
		return;
	}
	bMatchAbortHandled = true;

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			if (ANF_MatchLobbyPlayerController* MatchPC = Cast<ANF_MatchLobbyPlayerController>(PC))
			{
				MatchPC->ClientHandleMatchAborted(Reason);
			}
			PC->ClientReturnToMainMenuWithTextReason(FText::FromString(Reason));
		}
	}

	IOnlineSessionPtr SessionInterface;
	if (IOnlineSubsystem* OSS = Online::GetSubsystem(World))
	{
		SessionInterface = OSS->GetSessionInterface();
	}
	if (SessionInterface.IsValid() && SessionInterface->GetNamedSession(MatchSessionName))
	{
		DestroySessionCompleteHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
			FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleSessionDestroyed));
		if (SessionInterface->DestroySession(MatchSessionName))
		{
			return;
		}
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
		DestroySessionCompleteHandle.Reset();
	}
	ScheduleDedicatedServerExit();
}

void UNF_DedicatedMatchSubsystem::HandleSessionDestroyed(FName SessionName, bool bWasSuccessful)
{
	if (IOnlineSubsystem* OSS = Online::GetSubsystem(GetWorld()))
	{
		if (IOnlineSessionPtr SessionInterface = OSS->GetSessionInterface(); SessionInterface.IsValid()
			&& DestroySessionCompleteHandle.IsValid())
		{
			SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
		}
	}
	DestroySessionCompleteHandle.Reset();
	if (!bWasSuccessful)
	{
		UE_LOG(LogProject_NF, Warning, TEXT("Dedicated match session destruction failed: %s"), *SessionName.ToString());
	}
	ScheduleDedicatedServerExit();
}

void UNF_DedicatedMatchSubsystem::ScheduleDedicatedServerExit()
{
	if (!IsRunningDedicatedServer())
	{
		UE_LOG(LogProject_NF, Display, TEXT("Match abort handled; editor/listen server remains running."));
		return;
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(DedicatedServerExitTimerHandle, this,
			&ThisClass::ExitDedicatedServer, ServerExitDelaySeconds, false);
	}
}

void UNF_DedicatedMatchSubsystem::ExitDedicatedServer()
{
	if (IsRunningDedicatedServer())
	{
		FPlatformMisc::RequestExit(false);
	}
}
