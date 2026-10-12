// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "NF_MatchRules.h"
#include "NF_MatchmakingSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FNFMatchQueueEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FName QueueId;

	UPROPERTY(BlueprintReadOnly)
	FString PartyId;

	UPROPERTY(BlueprintReadOnly)
	FString PartyLeaderId;

	UPROPERTY(BlueprintReadOnly)
	TArray<FString> PlayerIds;

	UPROPERTY(BlueprintReadOnly)
	TArray<FDateTime> QueueJoinTimes;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNFMatchStateChanged, ENFMatchState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNFQueueCountChanged, int32, PlayerCount, int32, RequiredPlayers);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNFMatchmakingFailed, const FString&, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNFMatchTeamsAssigned, const TArray<FNFMatchTeamAssignment>&, Teams);

/** Local matchmaking coordinator backed by a Steam OSS lobby session. */
UCLASS()
class UNF_MatchmakingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Matchmaking|Party")
	bool CreateParty(const FString& LeaderId, FString& OutPartyId);

	UFUNCTION(BlueprintCallable, Category = "Matchmaking|Party")
	bool JoinParty(const FString& PartyId, const FString& PlayerId);

	UFUNCTION(BlueprintCallable, Category = "Matchmaking|Party")
	bool InviteFriendToParty(const FString& FriendSteamId);

	UFUNCTION(BlueprintCallable, Category = "Matchmaking|Party")
	bool LeaveParty(const FString& PlayerId);

	UFUNCTION(BlueprintCallable, Category = "Matchmaking|Queue")
	bool StartMatchmaking(const FString& PlayerId);

	/** Starts matchmaking in a registered queue. Existing callers continue to use the default queue above. */
	UFUNCTION(BlueprintCallable, Category = "Matchmaking|Queue")
	bool StartMatchmakingInQueue(const FString& PlayerId, FName QueueId);

	/** Adds or replaces a queue definition. Each client build must register the same definition for its QueueId. */
	UFUNCTION(BlueprintCallable, Category = "Matchmaking|Queue")
	bool RegisterMatchQueueDefinition(const FNFMatchQueueDefinition& Definition);

	UFUNCTION(BlueprintPure, Category = "Matchmaking|Queue")
	TArray<FNFMatchQueueDefinition> GetAvailableMatchQueues() const;

	UFUNCTION(BlueprintPure, Category = "Matchmaking|Queue")
	FName GetCurrentQueueId() const { return ActiveQueueDefinition.QueueId; }

	UFUNCTION(BlueprintCallable, Category = "Matchmaking|Queue")
	void CancelMatchmaking(const FString& PlayerId);

	UFUNCTION(BlueprintCallable, Category = "Matchmaking|Queue")
	bool StartLocalMatchmaking();

	UFUNCTION(BlueprintCallable, Category = "Matchmaking|Queue")
	bool StartLocalMatchmakingInQueue(FName QueueId);

	UFUNCTION(BlueprintCallable, Category = "Matchmaking|Queue")
	void CancelLocalMatchmaking();

	/** Receives a future dedicated-server allocation result. This stores the address as a handoff hook; travel is not performed yet. */
	UFUNCTION(BlueprintCallable, Category = "Matchmaking|Server")
	bool HandleDedicatedServerReady(const FString& ConnectAddress);

	UFUNCTION(BlueprintPure, Category = "Matchmaking")
	ENFMatchState GetMatchState() const { return MatchState; }

	UFUNCTION(BlueprintPure, Category = "Matchmaking|Queue")
	int32 GetQueuedPlayerCount() const { return QueuedPlayerCount; }

	UFUNCTION(BlueprintPure, Category = "Matchmaking|Queue")
	int32 GetQueueElapsedSeconds() const;

	UPROPERTY(BlueprintAssignable, Category = "Matchmaking")
	FNFMatchStateChanged OnMatchStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Matchmaking|Queue")
	FNFQueueCountChanged OnQueueCountChanged;

	UPROPERTY(BlueprintAssignable, Category = "Matchmaking")
	FNFMatchmakingFailed OnMatchmakingFailed;

	UPROPERTY(BlueprintAssignable, Category = "Matchmaking")
	FNFMatchTeamsAssigned OnMatchTeamsAssigned;

private:
	struct FPartyRecord
	{
		FString PartyId;
		FString LeaderId;
		TArray<FString> Members;
	};

	void SetMatchState(ENFMatchState NewState);
	void RecalculateQueue();
	void HandleQueueTimeout();
	void HandleFindSessionsComplete(bool bWasSuccessful);
	void HandleSessionInviteAccepted(const bool bWasSuccessful, const int32 LocalUserNum, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult);
	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleLobbyMemberJoined(FName SessionName, const FUniqueNetId& MemberId);
	void HandleLobbyMemberLeft(FName SessionName, const FUniqueNetId& MemberId, EOnSessionParticipantLeftReason Reason);
	void BeginLobbySearchOrCreate();
	void RefreshLobbyPlayerCount();
	void InvitePartyMembersToMatchLobby();
	void RefreshPartyRoster();
	void HandleSessionFailure(const FUniqueNetId& PlayerId, ESessionFailure::Type FailureType);
	void CleanupSession();
	void CancelWholeQueuedParty(const FString& PlayerId);
	FString MakeLocalPlayerId() const;
	void PublishCurrentLobbyMemberData(FName SessionName, const FString& QueueStartUtc = FString());

	TMap<FString, FPartyRecord> Parties;
	TMap<FString, FString> PlayerToParty;
	TArray<FNFMatchQueueEntry> QueueEntries;
	TMap<FName, FNFMatchQueueDefinition> MatchQueueDefinitions;
	FNFMatchQueueDefinition ActiveQueueDefinition;
	TMap<FString, FString> PlayerPartyId;
	TMap<FString, FString> PlayerPartyLeaderId;
	TMap<FString, FDateTime> PlayerQueueJoinTime;

	TSharedPtr<IOnlineSession, ESPMode::ThreadSafe> SessionInterface;
	TSharedPtr<FOnlineSessionSearch> SessionSearch;
	FDelegateHandle FindSessionsCompleteHandle;
	FDelegateHandle CreateSessionCompleteHandle;
	FDelegateHandle JoinSessionCompleteHandle;
	FDelegateHandle DestroySessionCompleteHandle;
	FDelegateHandle SessionParticipantJoinedHandle;
	FDelegateHandle SessionParticipantLeftHandle;
	FDelegateHandle SessionFailureHandle;
	FDelegateHandle SessionUserInviteAcceptedHandle;
	FTimerHandle QueueTimeoutTimerHandle;
	FTimerHandle LobbyRefreshTimerHandle;
	FTimerHandle PartyRefreshTimerHandle;
	FTimerHandle LobbySearchRetryTimerHandle;
	FDateTime QueueDeadline;
	FString LocalPartyId;
	FString LobbyOwnerId;
	FString PendingPartyId;
	FString PendingPartyLeaderId;
	FString PendingDedicatedServerAddress;
	FString MatchQueueStartUtc;
	TMap<FString, FString> MatchMemberPartyIds;
	TSet<FString> PreviousMatchMembers;
	int32 QueuedPlayerCount = 0;
	ENFMatchState MatchState = ENFMatchState::Ended;
	bool bCreateSessionInFlight = false;
	bool bIsLobbyLeader = false;
	bool bMatchLobbyAdmissionClosed = false;
	bool bTeamsAssigned = false;
	bool bJoiningPartyMatchInvite = false;
	int32 SearchRetryCount = 0;
};
