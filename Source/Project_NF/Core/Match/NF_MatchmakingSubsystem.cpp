// Copyright Epic Games, Inc. All Rights Reserved.

#include "NF_MatchmakingSubsystem.h"

#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "UObject/UObjectGlobals.h"
#include "Project_NF.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystemUtils.h"
#include "steam/steam_api.h"

namespace
{
	const FName MatchSessionName = NAME_GameSession;
	const FName PartySessionName(TEXT("NFPartySession"));
	const FName PartyIdKey(TEXT("NF_PARTY_ID"));
	const FName PartyLeaderKey(TEXT("NF_PARTY_LEADER"));
	const FName MatchmakingSessionKey(TEXT("NF_MATCHMAKING"));
	const FName LobbyKindKey(TEXT("NF_LOBBY_KIND"));
	const FName MatchQueueIdKey(TEXT("NF_QUEUE_ID"));
	const FName MatchRulesVersionKey(TEXT("NF_RULES_VERSION"));
	const ANSICHAR* MemberPartyIdKey = "NF_PARTY_ID";
	const ANSICHAR* MemberPartyLeaderKey = "NF_PARTY_LEADER";
	const ANSICHAR* MemberQueueStartKey = "NF_QUEUE_START_UTC";
	const ANSICHAR* MemberCanceledPartyKey = "NF_CANCELED_PARTY_ID";
	const ANSICHAR* LobbyPartyCanceledKey = "NF_CANCELED_PARTY_ID";
	const ANSICHAR* LobbyMatchStateKey = "NF_MATCH_STATE";
	const ANSICHAR* LobbyMatchTeamsKey = "NF_MATCH_TEAMS";
	const ANSICHAR* LobbyQueueIdKey = "NF_QUEUE_ID";

	uint64 GetSteamLobbyId(IOnlineSessionPtr Sessions, FName SessionName)
	{
		const FNamedOnlineSession* NamedSession = Sessions.IsValid() ? Sessions->GetNamedSession(SessionName) : nullptr;
		return NamedSession ? FCString::Strtoui64(*NamedSession->GetSessionIdStr(), nullptr, 10) : 0;
	}

	bool SetSteamLobbyData(uint64 LobbyId, const ANSICHAR* Key, const FString& Value)
	{
		return LobbyId != 0 && SteamMatchmaking() && SteamMatchmaking()->SetLobbyData(CSteamID(LobbyId), Key, TCHAR_TO_UTF8(*Value));
	}

	FString GetSteamLobbyData(uint64 LobbyId, const ANSICHAR* Key)
	{
		if (LobbyId == 0 || !SteamMatchmaking()) return FString();
		const char* Value = SteamMatchmaking()->GetLobbyData(CSteamID(LobbyId), Key);
		return Value ? UTF8_TO_TCHAR(Value) : FString();
	}

	FString GetSteamLobbyMemberData(uint64 LobbyId, uint64 MemberId, const ANSICHAR* Key)
	{
		if (LobbyId == 0 || MemberId == 0 || !SteamMatchmaking()) return FString();
		const char* Value = SteamMatchmaking()->GetLobbyMemberData(CSteamID(LobbyId), CSteamID(MemberId), Key);
		return Value ? UTF8_TO_TCHAR(Value) : FString();
	}

	FString SerializeTeams(const TArray<FNFMatchTeamAssignment>& Teams)
	{
		TArray<FString> SerializedTeams;
		for (const FNFMatchTeamAssignment& Team : Teams)
		{
			TArray<FString> Members;
			for (const FNFQueuedPlayer& Player : Team.Players)
			{
				Members.Add(FString::Printf(TEXT("%s,%s,%s"), *Player.PlayerId, *Player.PartyId, *Player.PartyLeaderId));
			}
			SerializedTeams.Add(FString::Join(Members, TEXT(";")));
		}
		return FString::Join(SerializedTeams, TEXT("|"));
	}

	bool DeserializeTeams(const FString& Value, const FNFMatchQueueDefinition& Definition, TArray<FNFMatchTeamAssignment>& OutTeams)
	{
		OutTeams.Reset();
		TArray<FString> SerializedTeams;
		Value.ParseIntoArray(SerializedTeams, TEXT("|"), false);
		if (!NF_IsValidMatchQueueDefinition(Definition) || SerializedTeams.Num() != Definition.TeamCount) return false;
		TArray<FNFQueuedPlayer> AllPlayers;
		for (const FString& SerializedTeam : SerializedTeams)
		{
			FNFMatchTeamAssignment& Team = OutTeams.AddDefaulted_GetRef();
			TArray<FString> SerializedMembers;
			SerializedTeam.ParseIntoArray(SerializedMembers, TEXT(";"), false);
			if (SerializedMembers.Num() != Definition.PlayersPerTeam) return false;
			for (const FString& SerializedMember : SerializedMembers)
			{
				TArray<FString> Fields;
				SerializedMember.ParseIntoArray(Fields, TEXT(","), false);
				if (Fields.Num() != 3) return false;
				FNFQueuedPlayer& Player = Team.Players.AddDefaulted_GetRef();
				Player.PlayerId = Fields[0];
				Player.PartyId = Fields[1];
				Player.PartyLeaderId = Fields[2];
				AllPlayers.Add(Player);
			}
		}
		TArray<FNFMatchTeamAssignment> ValidatedTeams;
		if (!NF_AssignMatchTeams(Definition, AllPlayers, ValidatedTeams))
		{
			OutTeams.Reset();
			return false;
		}
		// Validate party members have not been split across the published teams.
		TMap<FString, int32> PartyTeams;
		for (int32 TeamIndex = 0; TeamIndex < OutTeams.Num(); ++TeamIndex)
		{
			for (const FNFQueuedPlayer& Player : OutTeams[TeamIndex].Players)
			{
				if (const int32* ExistingTeam = PartyTeams.Find(Player.PartyId))
				{
					if (*ExistingTeam != TeamIndex) return false;
				}
				else
				{
					PartyTeams.Add(Player.PartyId, TeamIndex);
				}
			}
		}
		return true;
	}
}

void UNF_MatchmakingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &ThisClass::HandlePostLoadMap);

	RegisterMatchQueueDefinition(FNFMatchQueueDefinition());
	if (IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld()))
	{
		SessionInterface = OnlineSubsystem->GetSessionInterface();
	}
	if (SessionInterface.IsValid())
	{
		SessionParticipantJoinedHandle = SessionInterface->AddOnSessionParticipantJoinedDelegate_Handle(
			FOnSessionParticipantJoinedDelegate::CreateUObject(this, &ThisClass::HandleLobbyMemberJoined));
		SessionParticipantLeftHandle = SessionInterface->AddOnSessionParticipantLeftDelegate_Handle(
			FOnSessionParticipantLeftDelegate::CreateUObject(this, &ThisClass::HandleLobbyMemberLeft));
		SessionFailureHandle = SessionInterface->AddOnSessionFailureDelegate_Handle(
			FOnSessionFailureDelegate::CreateUObject(this, &ThisClass::HandleSessionFailure));
		SessionUserInviteAcceptedHandle = SessionInterface->AddOnSessionUserInviteAcceptedDelegate_Handle(
			FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &ThisClass::HandleSessionInviteAccepted));
	}
}

void UNF_MatchmakingSubsystem::Deinitialize()
{
	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(QueueTimeoutTimerHandle);
		World->GetTimerManager().ClearTimer(LobbyRefreshTimerHandle);
		World->GetTimerManager().ClearTimer(PartyRefreshTimerHandle);
		World->GetTimerManager().ClearTimer(LobbySearchRetryTimerHandle);
	}
	CleanupSession();
	Super::Deinitialize();
}

void UNF_MatchmakingSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (!LoadedWorld)
	{
		return;
	}

	const FString MapPackageName = LoadedWorld->GetOutermost()->GetName();
	UE_LOG(LogProject_NF, Warning,
		TEXT("[MapDiag] Loaded map: Package=%s World=%s NetMode=%d"),
		*MapPackageName,
		*LoadedWorld->GetMapName(),
		static_cast<int32>(LoadedWorld->GetNetMode()));
}

bool UNF_MatchmakingSubsystem::CreateParty(const FString& LeaderId, FString& OutPartyId)
{
	const FString LocalPlayerId = MakeLocalPlayerId();
	if (!SessionInterface.IsValid() || LocalPlayerId.IsEmpty() || (!LeaderId.IsEmpty() && LeaderId != LocalPlayerId) || PlayerToParty.Contains(LocalPlayerId) || (MatchState != ENFMatchState::Ended && MatchState != ENFMatchState::Canceled) || SessionInterface->GetNamedSession(PartySessionName))
	{
		return false;
	}

	FPartyRecord Party;
	Party.PartyId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	Party.LeaderId = LocalPlayerId;
	Party.Members.Add(LocalPlayerId);
	OutPartyId = Party.PartyId;
	PlayerToParty.Add(LocalPlayerId, Party.PartyId);
	PlayerPartyId.Add(LocalPlayerId, Party.PartyId);
	PlayerPartyLeaderId.Add(LocalPlayerId, LocalPlayerId);
	Parties.Add(Party.PartyId, MoveTemp(Party));
	LocalPartyId = OutPartyId;

	FOnlineSessionSettings PartySettings;
	PartySettings.NumPublicConnections = NF_MatchRules::MaxPartySize;
	PartySettings.bShouldAdvertise = true;
	PartySettings.bAllowJoinInProgress = true;
	PartySettings.bAllowJoinViaPresence = true;
	PartySettings.bAllowJoinViaPresenceFriendsOnly = true;
	PartySettings.bUsesPresence = true;
	PartySettings.bAllowInvites = true;
	PartySettings.bUseLobbiesIfAvailable = true;
	PartySettings.Set(LobbyKindKey, FString(TEXT("Party")), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	PartySettings.Set(PartyIdKey, LocalPartyId, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	PartySettings.Set(PartyLeaderKey, LocalPlayerId, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	CreateSessionCompleteHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleCreateSessionComplete));
	if (!SessionInterface->CreateSession(0, PartySessionName, PartySettings))
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
		Parties.Remove(LocalPartyId);
		PlayerToParty.Remove(LocalPlayerId);
		LocalPartyId.Reset();
		return false;
	}
	return true;
}

bool UNF_MatchmakingSubsystem::JoinParty(const FString& PartyId, const FString& PlayerId)
{
	const FString LocalPlayerId = MakeLocalPlayerId();
	if (!SessionInterface.IsValid() || PartyId.IsEmpty() || LocalPlayerId.IsEmpty() || (!PlayerId.IsEmpty() && PlayerId != LocalPlayerId) || PlayerToParty.Contains(LocalPlayerId) || (MatchState != ENFMatchState::Ended && MatchState != ENFMatchState::Canceled) || SessionInterface->GetNamedSession(PartySessionName))
	{
		return false;
	}
	PendingPartyId = PartyId;
	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->bIsLanQuery = false;
	SessionSearch->MaxSearchResults = 50;
	SessionSearch->QuerySettings.Set(LobbyKindKey, FString(TEXT("Party")), EOnlineComparisonOp::Equals);
	SessionSearch->QuerySettings.Set(PartyIdKey, PartyId, EOnlineComparisonOp::Equals);
	FindSessionsCompleteHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::HandleFindSessionsComplete));
	if (!SessionInterface->FindSessions(0, SessionSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
		SessionSearch.Reset();
		PendingPartyId.Reset();
		return false;
	}
	return true;
}

bool UNF_MatchmakingSubsystem::InviteFriendToParty(const FString& FriendSteamId)
{
	const FPartyRecord* Party = Parties.Find(LocalPartyId);
	if (!SessionInterface.IsValid() || !Party || Party->LeaderId != MakeLocalPlayerId() || FriendSteamId.IsEmpty() || MatchState != ENFMatchState::Ended || !SessionInterface->GetNamedSession(PartySessionName))
	{
		return false;
	}
	const uint64 LobbyId = GetSteamLobbyId(SessionInterface, PartySessionName);
	if (LobbyId == 0 || !SteamMatchmaking() || SteamMatchmaking()->GetNumLobbyMembers(CSteamID(LobbyId)) >= NF_MatchRules::MaxPartySize)
	{
		return false;
	}
	IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld());
	const IOnlineIdentityPtr Identity = OnlineSubsystem ? OnlineSubsystem->GetIdentityInterface() : nullptr;
	const FUniqueNetIdPtr FriendId = Identity.IsValid() ? Identity->CreateUniquePlayerId(FriendSteamId) : nullptr;
	if (!FriendId.IsValid() || Party->Members.Contains(FriendSteamId)) return false;
	return SessionInterface->SendSessionInviteToFriend(0, PartySessionName, *FriendId);
}

bool UNF_MatchmakingSubsystem::LeaveParty(const FString& PlayerId)
{
	const FString LocalPlayerId = MakeLocalPlayerId();
	if (!PlayerId.IsEmpty() && PlayerId != LocalPlayerId)
	{
		return false;
	}
	const FString* PartyId = PlayerToParty.Find(LocalPlayerId);
	if (!PartyId)
	{
		return false;
	}

	FPartyRecord* Party = Parties.Find(*PartyId);
	if (!Party)
	{
		PlayerToParty.Remove(PlayerId);
		return false;
	}

	const FString PartyKey = *PartyId;
	const bool bWasLeader = Party->LeaderId == LocalPlayerId;
	if (MatchState == ENFMatchState::Queueing || MatchState == ENFMatchState::LobbyWaiting)
	{
		CancelMatchmaking(LocalPlayerId);
	}
	for (const FString& Member : Party->Members)
	{
		PlayerToParty.Remove(Member);
	}
	Parties.Remove(PartyKey);
	if (LocalPartyId == PartyKey)
	{
		LocalPartyId.Reset();
	}

	if (SessionInterface.IsValid() && SessionInterface->GetNamedSession(PartySessionName))
	{
		if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(PartyRefreshTimerHandle);
		SessionInterface->DestroySession(PartySessionName);
	}
	return true;
}

bool UNF_MatchmakingSubsystem::StartMatchmaking(const FString& PlayerId)
{
	return StartMatchmakingInQueue(PlayerId, NF_MatchRules::DefaultQueueId);
}

bool UNF_MatchmakingSubsystem::StartMatchmakingInQueue(const FString& PlayerId, FName QueueId)
{
	const FNFMatchQueueDefinition* QueueDefinition = MatchQueueDefinitions.Find(QueueId);
	if (!QueueDefinition)
	{
		OnMatchmakingFailed.Broadcast(FString::Printf(TEXT("등록되지 않은 매치 대기열입니다: %s"), *QueueId.ToString()));
		return false;
	}
	if (!SessionInterface.IsValid() || PlayerId.IsEmpty() || PlayerId != MakeLocalPlayerId() || (MatchState != ENFMatchState::Ended && MatchState != ENFMatchState::Canceled))
	{
		return false;
	}
	ActiveQueueDefinition = *QueueDefinition;
	if (!LocalPartyId.IsEmpty()) RefreshPartyRoster();

	FNFMatchQueueEntry Entry;
	Entry.QueueId = ActiveQueueDefinition.QueueId;
	if (const FString* PartyId = PlayerToParty.Find(PlayerId))
	{
		FPartyRecord* Party = Parties.Find(*PartyId);
		if (!Party || Party->LeaderId != PlayerId || Party->Members.Num() > ActiveQueueDefinition.MaxPartySize || !SessionInterface->GetNamedSession(PartySessionName))
		{
			return false;
		}
		Entry.PartyId = Party->PartyId;
		Entry.PartyLeaderId = Party->LeaderId;
		Entry.PlayerIds = Party->Members;
	}
	else
	{
		Entry.PartyId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
		Entry.PartyLeaderId = PlayerId;
		Entry.PlayerIds.Add(PlayerId);
	}
	LocalPartyId = Entry.PartyId;
	bTeamsAssigned = false;
	PendingMatchTeams.Reset();
	bMatchLobbyAdmissionClosed = false;
	PendingDedicatedServerAddress.Reset();

	const FDateTime Now = FDateTime::UtcNow();
	MatchQueueStartUtc = LexToString(Now.ToUnixTimestamp());
	if (PlayerToParty.Contains(PlayerId) && !SetSteamLobbyData(GetSteamLobbyId(SessionInterface, PartySessionName), "NF_PARTY_QUEUE_START_UTC", MatchQueueStartUtc))
	{
		OnMatchmakingFailed.Broadcast(TEXT("파티 대기 시작 시각을 Steam Lobby에 기록하지 못했습니다."));
		return false;
	}
	for (const FString& MemberId : Entry.PlayerIds)
	{
		Entry.QueueJoinTimes.Add(Now);
		PlayerPartyId.Add(MemberId, Entry.PartyId);
		PlayerPartyLeaderId.Add(MemberId, Entry.PartyLeaderId);
		PlayerQueueJoinTime.Add(MemberId, Now);
	}
	QueueDeadline = Now + FTimespan::FromSeconds(ActiveQueueDefinition.QueueTimeoutSeconds);
	QueueEntries.Add(MoveTemp(Entry));
	SearchRetryCount = 0;
	SetMatchState(ENFMatchState::Queueing);
	RecalculateQueue();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(QueueTimeoutTimerHandle, this, &ThisClass::HandleQueueTimeout, static_cast<float>(ActiveQueueDefinition.QueueTimeoutSeconds), false);
	}

	FUniqueNetIdPtr LocalUserId;
	if (IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld()))
	{
		if (IOnlineIdentityPtr Identity = OnlineSubsystem->GetIdentityInterface())
		{
			LocalUserId = Identity->GetUniquePlayerId(0);
		}
	}
	if (!LocalUserId.IsValid())
	{
		CancelMatchmaking(PlayerId);
		OnMatchmakingFailed.Broadcast(TEXT("Steam 로그인 사용자 ID를 가져오지 못했습니다."));
		return false;
	}

	PublishCurrentLobbyMemberData(PartySessionName);
	BeginLobbySearchOrCreate();
	return true;
}

void UNF_MatchmakingSubsystem::CancelMatchmaking(const FString& PlayerId)
{
	const FString CancelingPartyId = PlayerPartyId.FindRef(PlayerId).IsEmpty() ? LocalPartyId : PlayerPartyId.FindRef(PlayerId);
	const uint64 MatchLobbyId = GetSteamLobbyId(SessionInterface, MatchSessionName);
	if (MatchLobbyId != 0 && !CancelingPartyId.IsEmpty() && SteamMatchmaking())
	{
		SteamMatchmaking()->SetLobbyMemberData(CSteamID(MatchLobbyId), MemberCanceledPartyKey, TCHAR_TO_UTF8(*CancelingPartyId));
		if (bIsLobbyLeader)
		{
			SetSteamLobbyData(MatchLobbyId, LobbyPartyCanceledKey, CancelingPartyId);
		}
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(QueueTimeoutTimerHandle);
		World->GetTimerManager().ClearTimer(LobbyRefreshTimerHandle);
	}
	if (SessionInterface.IsValid() && FindSessionsCompleteHandle.IsValid())
	{
		SessionInterface->CancelFindSessions();
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
		SessionSearch.Reset();
	}
	CancelWholeQueuedParty(PlayerId);
	if (SessionInterface.IsValid() && SessionInterface->GetNamedSession(MatchSessionName))
	{
		DestroySessionCompleteHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleDestroySessionComplete));
		SessionInterface->DestroySession(MatchSessionName);
	}
	SetMatchState(ENFMatchState::Canceled);
	bIsLobbyLeader = false;
	bTeamsAssigned = false;
	PendingMatchTeams.Reset();
	PendingDedicatedServerAddress.Reset();
	LobbyOwnerId.Reset();
}

bool UNF_MatchmakingSubsystem::StartLocalMatchmaking()
{
	const FString LocalPlayerId = MakeLocalPlayerId();
	return !LocalPlayerId.IsEmpty() && StartMatchmaking(LocalPlayerId);
}

bool UNF_MatchmakingSubsystem::StartLocalMatchmakingInQueue(FName QueueId)
{
	const FString LocalPlayerId = MakeLocalPlayerId();
	return !LocalPlayerId.IsEmpty() && StartMatchmakingInQueue(LocalPlayerId, QueueId);
}

bool UNF_MatchmakingSubsystem::RegisterMatchQueueDefinition(const FNFMatchQueueDefinition& Definition)
{
	if (!NF_IsValidMatchQueueDefinition(Definition))
	{
		UE_LOG(LogProject_NF, Warning, TEXT("Rejected invalid matchmaking queue definition: %s"), *Definition.QueueId.ToString());
		return false;
	}
	MatchQueueDefinitions.Add(Definition.QueueId, Definition);
	return true;
}

TArray<FNFMatchQueueDefinition> UNF_MatchmakingSubsystem::GetAvailableMatchQueues() const
{
	TArray<FNFMatchQueueDefinition> Definitions;
	MatchQueueDefinitions.GenerateValueArray(Definitions);
	Definitions.Sort([](const FNFMatchQueueDefinition& A, const FNFMatchQueueDefinition& B)
	{
		return A.QueueId.ToString() < B.QueueId.ToString();
	});
	return Definitions;
}

void UNF_MatchmakingSubsystem::CancelLocalMatchmaking()
{
	CancelMatchmaking(MakeLocalPlayerId());
}

bool UNF_MatchmakingSubsystem::HandleDedicatedServerReady(const FString& ConnectAddress)
{
	if (MatchState != ENFMatchState::ServerPreparing || !bTeamsAssigned)
	{
		UE_LOG(LogProject_NF, Warning, TEXT("Dedicated server allocation arrived before this match was ready."));
		return false;
	}

	if (ConnectAddress.IsEmpty())
	{
		UE_LOG(LogProject_NF, Warning, TEXT("Dedicated server allocation returned an empty connect address."));
		return false;
	}

	PendingDedicatedServerAddress = ConnectAddress;
	UE_LOG(LogProject_NF, Display, TEXT("Dedicated server address stored for match handoff. Player travel is not implemented yet: %s"), *PendingDedicatedServerAddress);
	return true;
}

bool UNF_MatchmakingSubsystem::GetPendingMatchTeams(TArray<FNFMatchTeamAssignment>& OutTeams) const
{
	OutTeams = PendingMatchTeams;
	return bTeamsAssigned && !PendingMatchTeams.IsEmpty();
}

FString UNF_MatchmakingSubsystem::GetLocalMatchPlayerId() const
{
	return MakeLocalPlayerId();
}

void UNF_MatchmakingSubsystem::HandleDedicatedMatchAborted(const FString& Reason)
{
	PendingMatchTeams.Reset();
	PendingDedicatedServerAddress.Reset();
	bTeamsAssigned = false;
	CancelMatchmaking(MakeLocalPlayerId());
	OnMatchmakingFailed.Broadcast(Reason);
}

void UNF_MatchmakingSubsystem::PublishMatchTeamsAssigned(const TArray<FNFMatchTeamAssignment>& Teams)
{
	PendingMatchTeams = Teams;
	bTeamsAssigned = !PendingMatchTeams.IsEmpty();
	OnMatchTeamsAssigned.Broadcast(PendingMatchTeams);
}

int32 UNF_MatchmakingSubsystem::GetQueueElapsedSeconds() const
{
	if (MatchState != ENFMatchState::Queueing && MatchState != ENFMatchState::LobbyWaiting && MatchState != ENFMatchState::ServerPreparing)
	{
		return 0;
	}
	const FDateTime QueueStart = QueueDeadline - FTimespan::FromSeconds(ActiveQueueDefinition.QueueTimeoutSeconds);
	return FMath::Max(0, static_cast<int32>((FDateTime::UtcNow() - QueueStart).GetTotalSeconds()));
}

void UNF_MatchmakingSubsystem::SetMatchState(ENFMatchState NewState)
{
	if (MatchState != NewState)
	{
		MatchState = NewState;
		OnMatchStateChanged.Broadcast(MatchState);
	}
}

void UNF_MatchmakingSubsystem::RecalculateQueue()
{
	QueuedPlayerCount = 0;
	for (const FNFMatchQueueEntry& Entry : QueueEntries)
	{
		QueuedPlayerCount += Entry.PlayerIds.Num();
	}
	OnQueueCountChanged.Broadcast(QueuedPlayerCount, ActiveQueueDefinition.PlayerCount);

	if (QueuedPlayerCount >= ActiveQueueDefinition.PlayerCount)
	{
		// Lobby member metadata must be validated by the owner before any client enters preparation.
		return;
	}

	if (MatchState == ENFMatchState::Queueing)
	{
		SetMatchState(ENFMatchState::LobbyWaiting);
	}
}

void UNF_MatchmakingSubsystem::HandleQueueTimeout()
{
	if (MatchState == ENFMatchState::Queueing || MatchState == ENFMatchState::LobbyWaiting)
	{
		OnMatchmakingFailed.Broadcast(TEXT("이 플레이어(또는 파티)가 대기열 참가 후 3분 내에 8명이 모이지 않아 매칭이 취소되었습니다."));
		CancelMatchmaking(MakeLocalPlayerId());
	}
}

void UNF_MatchmakingSubsystem::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionName != MatchSessionName && SessionName != PartySessionName)
	{
		return;
	}
	SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
	bCreateSessionInFlight = false;
	if (SessionName == MatchSessionName && MatchState == ENFMatchState::Canceled)
	{
		SessionInterface->DestroySession(MatchSessionName);
		return;
	}
	if (!bWasSuccessful)
	{
		if (SessionName == PartySessionName)
		{
			PlayerToParty.Remove(MakeLocalPlayerId());
			PlayerPartyId.Remove(MakeLocalPlayerId());
			PlayerPartyLeaderId.Remove(MakeLocalPlayerId());
			Parties.Remove(LocalPartyId);
			LocalPartyId.Reset();
			OnMatchmakingFailed.Broadcast(TEXT("Steam 파티 Lobby 생성에 실패했습니다."));
		}
		else
		{
			CancelMatchmaking(MakeLocalPlayerId());
			OnMatchmakingFailed.Broadcast(TEXT("Steam 매치 Lobby 생성에 실패했습니다."));
		}
		return;
	}
	if (SessionName == PartySessionName)
	{
		PublishCurrentLobbyMemberData(PartySessionName);
		if (UWorld* World = GetWorld()) World->GetTimerManager().SetTimer(PartyRefreshTimerHandle, this, &ThisClass::RefreshPartyRoster, 1.0f, true);
		return;
	}
	bIsLobbyLeader = true;
	LobbyOwnerId = MakeLocalPlayerId();
	SetSteamLobbyData(GetSteamLobbyId(SessionInterface, MatchSessionName), LobbyMatchStateKey, TEXT("LobbyWaiting"));
	PublishCurrentLobbyMemberData(MatchSessionName, MatchQueueStartUtc);
	InvitePartyMembersToMatchLobby();
	RefreshLobbyPlayerCount();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(LobbyRefreshTimerHandle, this, &ThisClass::RefreshLobbyPlayerCount, 1.0f, true);
	}
	SetMatchState(ENFMatchState::LobbyWaiting);
}

void UNF_MatchmakingSubsystem::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if ((SessionName == MatchSessionName || SessionName == PartySessionName) && SessionInterface.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
	}
	if (SessionName == MatchSessionName && MatchState == ENFMatchState::Canceled)
	{
		if (SessionInterface->GetNamedSession(MatchSessionName)) SessionInterface->DestroySession(MatchSessionName);
		return;
	}
	if ((SessionName == MatchSessionName || SessionName == PartySessionName) && Result != EOnJoinSessionCompleteResult::Success)
	{
		if (SessionName == PartySessionName)
		{
			PendingPartyId.Reset();
			OnMatchmakingFailed.Broadcast(TEXT("Steam 파티 Lobby 참가에 실패했습니다."));
		}
		else
		{
			if (bJoiningPartyMatchInvite)
			{
				bJoiningPartyMatchInvite = false;
				OnMatchmakingFailed.Broadcast(TEXT("파티원이 매치 Lobby에 참가하지 못해 파티 대기열을 취소했습니다."));
				CancelMatchmaking(MakeLocalPlayerId());
			}
			else
			{
				OnMatchmakingFailed.Broadcast(TEXT("Steam Match Lobby 참가에 실패했습니다. 다시 검색합니다."));
				++SearchRetryCount;
				BeginLobbySearchOrCreate();
			}
		}
	}
	else if (SessionName == PartySessionName && Result == EOnJoinSessionCompleteResult::Success)
	{
		const uint64 PartyLobbyId = GetSteamLobbyId(SessionInterface, PartySessionName);
		LocalPartyId = PendingPartyId.IsEmpty() ? GetSteamLobbyData(PartyLobbyId, "NF_PARTY_ID") : PendingPartyId;
		PendingPartyId.Reset();
		PendingPartyLeaderId = GetSteamLobbyData(PartyLobbyId, "NF_PARTY_LEADER");
		if (PartyLobbyId == 0 || LocalPartyId.IsEmpty() || PendingPartyLeaderId.IsEmpty() || !SteamMatchmaking() || SteamMatchmaking()->GetNumLobbyMembers(CSteamID(PartyLobbyId)) > NF_MatchRules::MaxPartySize)
		{
			OnMatchmakingFailed.Broadcast(TEXT("파티 Lobby 메타데이터 또는 최대 인원이 유효하지 않습니다."));
			SessionInterface->DestroySession(PartySessionName);
			LocalPartyId.Reset();
			return;
		}
		FPartyRecord Party;
		Party.PartyId = LocalPartyId;
		Party.LeaderId = PendingPartyLeaderId;
		Party.Members.Add(MakeLocalPlayerId());
		Parties.Add(LocalPartyId, MoveTemp(Party));
		PlayerToParty.Add(MakeLocalPlayerId(), LocalPartyId);
		PlayerPartyId.Add(MakeLocalPlayerId(), LocalPartyId);
		PlayerPartyLeaderId.Add(MakeLocalPlayerId(), PendingPartyLeaderId);
		PublishCurrentLobbyMemberData(PartySessionName);
		RefreshPartyRoster();
		if (UWorld* World = GetWorld()) World->GetTimerManager().SetTimer(PartyRefreshTimerHandle, this, &ThisClass::RefreshPartyRoster, 1.0f, true);
	}
	else if (SessionName == MatchSessionName && Result == EOnJoinSessionCompleteResult::Success)
	{
		const bool bJoinedThroughPartyInvite = bJoiningPartyMatchInvite;
		bJoiningPartyMatchInvite = false;
		bIsLobbyLeader = false;
		const uint64 JoinedMatchLobbyId = GetSteamLobbyId(SessionInterface, MatchSessionName);
		LobbyOwnerId = SteamMatchmaking() && JoinedMatchLobbyId != 0 ? LexToString(SteamMatchmaking()->GetLobbyOwner(CSteamID(JoinedMatchLobbyId)).ConvertToUint64()) : FString();
		const FString LocalId = MakeLocalPlayerId();
		const FString LocalQueueStart = GetSteamLobbyMemberData(JoinedMatchLobbyId, FCString::Strtoui64(*LocalId, nullptr, 10), MemberQueueStartKey);
		if (!LocalQueueStart.IsEmpty())
		{
			MatchQueueStartUtc = LocalQueueStart;
			QueueDeadline = FDateTime::FromUnixTimestamp(FCString::Atoi64(*LocalQueueStart)) + FTimespan::FromSeconds(ActiveQueueDefinition.QueueTimeoutSeconds);
		}
		SetMatchState(ENFMatchState::LobbyWaiting);
		PublishCurrentLobbyMemberData(MatchSessionName, MatchQueueStartUtc);
		if (!bJoinedThroughPartyInvite) InvitePartyMembersToMatchLobby();
		RefreshLobbyPlayerCount();
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(LobbyRefreshTimerHandle, this, &ThisClass::RefreshLobbyPlayerCount, 1.0f, true);
		}
	}
}

void UNF_MatchmakingSubsystem::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionName == MatchSessionName && SessionInterface.IsValid())
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
	}
}

void UNF_MatchmakingSubsystem::HandleLobbyMemberJoined(FName SessionName, const FUniqueNetId& MemberId)
{
	if (SessionName == PartySessionName)
	{
		RefreshPartyRoster();
		return;
	}
	if (SessionName != MatchSessionName || (MatchState != ENFMatchState::Queueing && MatchState != ENFMatchState::LobbyWaiting))
	{
		return;
	}
	RefreshLobbyPlayerCount();
}

void UNF_MatchmakingSubsystem::HandleLobbyMemberLeft(FName SessionName, const FUniqueNetId& MemberId, EOnSessionParticipantLeftReason Reason)
{
	if (SessionName == PartySessionName)
	{
		if (MatchState == ENFMatchState::Queueing || MatchState == ENFMatchState::LobbyWaiting)
		{
			const uint64 MatchLobbyId = GetSteamLobbyId(SessionInterface, MatchSessionName);
			if (MatchLobbyId != 0 && SteamMatchmaking())
			{
				SteamMatchmaking()->SetLobbyMemberData(CSteamID(MatchLobbyId), MemberCanceledPartyKey, TCHAR_TO_UTF8(*LocalPartyId));
				if (bIsLobbyLeader) SetSteamLobbyData(MatchLobbyId, LobbyPartyCanceledKey, LocalPartyId);
			}
			CancelMatchmaking(MakeLocalPlayerId());
		}
		else
		{
			RefreshPartyRoster();
		}
		return;
	}
	if (SessionName != MatchSessionName || (MatchState != ENFMatchState::Queueing && MatchState != ENFMatchState::LobbyWaiting))
	{
		return;
	}
	const FString LeavingPlayerId = MemberId.ToString();
	if (LeavingPlayerId == LobbyOwnerId)
	{
		OnMatchmakingFailed.Broadcast(TEXT("Steam Lobby 소유자가 이탈하여 매칭을 취소했습니다."));
		CancelMatchmaking(MakeLocalPlayerId());
		return;
	}
	if (const FString* DepartingPartyId = MatchMemberPartyIds.Find(LeavingPlayerId))
	{
		const uint64 CurrentLobbyId = GetSteamLobbyId(SessionInterface, MatchSessionName);
		if (bIsLobbyLeader)
		{
			SetSteamLobbyData(CurrentLobbyId, LobbyPartyCanceledKey, *DepartingPartyId);
		}
	}
	CancelWholeQueuedParty(LeavingPlayerId);
	if (LeavingPlayerId == MakeLocalPlayerId())
	{
		CancelMatchmaking(LeavingPlayerId);
		return;
	}
	RefreshLobbyPlayerCount();
}

void UNF_MatchmakingSubsystem::BeginLobbySearchOrCreate()
{
	if (!SessionInterface.IsValid())
	{
		CancelMatchmaking(MakeLocalPlayerId());
		return;
	}

	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->bIsLanQuery = false;
	SessionSearch->MaxSearchResults = 100;
	SessionSearch->QuerySettings.Set(MatchmakingSessionKey, true, EOnlineComparisonOp::Equals);
	SessionSearch->QuerySettings.Set(MatchQueueIdKey, ActiveQueueDefinition.QueueId.ToString(), EOnlineComparisonOp::Equals);
	SessionSearch->QuerySettings.Set(MatchRulesVersionKey, LexToString(ActiveQueueDefinition.RulesVersion), EOnlineComparisonOp::Equals);
	FindSessionsCompleteHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::HandleFindSessionsComplete));
	if (!SessionInterface->FindSessions(0, SessionSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
		SessionSearch.Reset();
		if (SearchRetryCount < 3)
		{
			++SearchRetryCount;
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimer(LobbySearchRetryTimerHandle, this, &ThisClass::BeginLobbySearchOrCreate, FMath::FRandRange(0.35f, 1.1f), false);
				return;
			}
		}
		OnMatchmakingFailed.Broadcast(TEXT("Steam Lobby 검색을 시작하지 못했습니다."));
		CancelMatchmaking(MakeLocalPlayerId());
	}
}

void UNF_MatchmakingSubsystem::HandleFindSessionsComplete(bool bWasSuccessful)
{
	if (!SessionInterface.IsValid())
	{
		return;
	}
	SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
	if (!PendingPartyId.IsEmpty())
	{
		if (bWasSuccessful && SessionSearch.IsValid())
		{
			for (const FOnlineSessionSearchResult& Result : SessionSearch->SearchResults)
			{
				FString FoundPartyId;
				FString FoundLobbyKind;
				Result.Session.SessionSettings.Get(PartyIdKey, FoundPartyId);
				Result.Session.SessionSettings.Get(LobbyKindKey, FoundLobbyKind);
				if (FoundPartyId == PendingPartyId && FoundLobbyKind == TEXT("Party"))
				{
					JoinSessionCompleteHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleJoinSessionComplete));
					if (SessionInterface->JoinSession(0, PartySessionName, Result))
					{
						SessionSearch.Reset();
						return;
					}
					SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
				}
			}
		}
		SessionSearch.Reset();
		PendingPartyId.Reset();
		OnMatchmakingFailed.Broadcast(TEXT("Steam 파티 Lobby를 찾지 못했습니다."));
		return;
	}
	const int32 RequiredConnections = FMath::Max(1, PlayerToParty.Contains(MakeLocalPlayerId()) && Parties.Contains(LocalPartyId) ? Parties.FindChecked(LocalPartyId).Members.Num() : 1);
	if (bWasSuccessful && SessionSearch.IsValid())
	{
		for (const FOnlineSessionSearchResult& Result : SessionSearch->SearchResults)
		{
			bool bIsMatchmakingLobby = false;
			FString FoundMatchState;
			FString FoundQueueId;
			FString FoundRulesVersion;
			Result.Session.SessionSettings.Get(MatchmakingSessionKey, bIsMatchmakingLobby);
			Result.Session.SessionSettings.Get(LobbyMatchStateKey, FoundMatchState);
			Result.Session.SessionSettings.Get(MatchQueueIdKey, FoundQueueId);
			Result.Session.SessionSettings.Get(MatchRulesVersionKey, FoundRulesVersion);
			if (!bIsMatchmakingLobby || FoundQueueId != ActiveQueueDefinition.QueueId.ToString() || FoundRulesVersion != LexToString(ActiveQueueDefinition.RulesVersion) || FoundMatchState != TEXT("LobbyWaiting") || Result.Session.NumOpenPublicConnections < RequiredConnections)
			{
				continue;
			}

			JoinSessionCompleteHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
				FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleJoinSessionComplete));
		if (SessionInterface->JoinSession(0, MatchSessionName, Result))
			{
				LobbyOwnerId = Result.Session.OwningUserId.IsValid() ? Result.Session.OwningUserId->ToString() : FString();
				SessionSearch.Reset();
				SearchRetryCount = 0;
				return;
			}
			SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
			if (SearchRetryCount < 3)
			{
				++SearchRetryCount;
				SessionSearch.Reset();
				BeginLobbySearchOrCreate();
				return;
			}
		}
	}
	SessionSearch.Reset();
	if (SearchRetryCount < 3)
	{
		++SearchRetryCount;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(LobbySearchRetryTimerHandle, this, &ThisClass::BeginLobbySearchOrCreate, FMath::FRandRange(0.35f, 1.1f), false);
			return;
		}
	}
	SearchRetryCount = 0;

	FOnlineSessionSettings Settings;
	Settings.NumPublicConnections = ActiveQueueDefinition.PlayerCount;
	Settings.NumPrivateConnections = 0;
	Settings.bShouldAdvertise = true;
	Settings.bAllowJoinInProgress = true;
	Settings.bAllowJoinViaPresence = true;
	Settings.bUsesPresence = true;
	Settings.bUseLobbiesIfAvailable = true;
	Settings.Set(MatchmakingSessionKey, true, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(MatchQueueIdKey, ActiveQueueDefinition.QueueId.ToString(), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(MatchRulesVersionKey, LexToString(ActiveQueueDefinition.RulesVersion), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(LobbyKindKey, FString(TEXT("Match")), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(PartyIdKey, LocalPartyId, EOnlineDataAdvertisementType::ViaOnlineService);
	Settings.Set(PartyLeaderKey, PlayerPartyLeaderId.FindRef(MakeLocalPlayerId()), EOnlineDataAdvertisementType::ViaOnlineService);
	Settings.Set(LobbyMatchStateKey, FString(TEXT("LobbyWaiting")), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	CreateSessionCompleteHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleCreateSessionComplete));
	bCreateSessionInFlight = true;
	if (!SessionInterface->CreateSession(0, MatchSessionName, Settings))
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
		bCreateSessionInFlight = false;
		OnMatchmakingFailed.Broadcast(TEXT("Steam Lobby 생성에 실패했습니다."));
		CancelMatchmaking(MakeLocalPlayerId());
	}
}

void UNF_MatchmakingSubsystem::HandleSessionInviteAccepted(const bool bWasSuccessful, const int32 LocalUserNum, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult)
{
	if (!bWasSuccessful || !SessionInterface.IsValid())
	{
		OnMatchmakingFailed.Broadcast(TEXT("Steam 초대를 확인하지 못했습니다."));
		return;
	}
	FString LobbyKind;
	FString PartyId;
	FString PartyLeaderId;
	InviteResult.Session.SessionSettings.Get(LobbyKindKey, LobbyKind);
	InviteResult.Session.SessionSettings.Get(PartyIdKey, PartyId);
	InviteResult.Session.SessionSettings.Get(PartyLeaderKey, PartyLeaderId);
	const FName TargetSessionName = LobbyKind == TEXT("Party") ? PartySessionName : MatchSessionName;
	if (LobbyKind != TEXT("Party") && LobbyKind != TEXT("Match"))
	{
		OnMatchmakingFailed.Broadcast(TEXT("Steam 초대에 로비 유형 정보가 없습니다."));
		return;
	}
	if (TargetSessionName == MatchSessionName)
	{
		FString InvitedMatchState;
		FString InvitedQueueId;
		FString InvitedRulesVersion;
		InviteResult.Session.SessionSettings.Get(LobbyMatchStateKey, InvitedMatchState);
		InviteResult.Session.SessionSettings.Get(MatchQueueIdKey, InvitedQueueId);
		InviteResult.Session.SessionSettings.Get(MatchRulesVersionKey, InvitedRulesVersion);
		const FName QueueId(*InvitedQueueId);
		const FNFMatchQueueDefinition* QueueDefinition = MatchQueueDefinitions.Find(QueueId);
		if (!QueueDefinition || QueueDefinition->RulesVersion != FCString::Atoi(*InvitedRulesVersion))
		{
			OnMatchmakingFailed.Broadcast(TEXT("초대받은 매치 대기열 규칙을 현재 클라이언트에서 찾지 못했습니다."));
			return;
		}
		ActiveQueueDefinition = *QueueDefinition;
		if (InvitedMatchState != TEXT("LobbyWaiting") || InviteResult.Session.NumOpenPublicConnections <= 0)
		{
			OnMatchmakingFailed.Broadcast(TEXT("매치 Lobby가 이미 시작 준비 중이거나 빈 자리가 없습니다."));
			return;
		}
	}
	if (SessionInterface->GetNamedSession(TargetSessionName))
	{
		return;
	}
	if (TargetSessionName == MatchSessionName && MatchState == ENFMatchState::Ended)
	{
		MatchQueueStartUtc = LexToString(FDateTime::UtcNow().ToUnixTimestamp());
	}

	if (TargetSessionName == PartySessionName)
	{
		PendingPartyId = PartyId;
		PendingPartyLeaderId = PartyLeaderId;
	}
	else
	{
		const uint64 PartyLobbyId = GetSteamLobbyId(SessionInterface, PartySessionName);
		if (PartyLobbyId == 0)
		{
			OnMatchmakingFailed.Broadcast(TEXT("매치 초대 처리 중 파티 Lobby가 연결되어 있지 않습니다."));
			return;
		}
		PartyId = GetSteamLobbyData(PartyLobbyId, "NF_PARTY_ID");
		PartyLeaderId = GetSteamLobbyData(PartyLobbyId, "NF_PARTY_LEADER");
		MatchQueueStartUtc = GetSteamLobbyData(PartyLobbyId, "NF_PARTY_QUEUE_START_UTC");
		if (PartyId.IsEmpty() || PartyLeaderId.IsEmpty())
		{
			OnMatchmakingFailed.Broadcast(TEXT("현재 Steam 파티 정보를 읽지 못했습니다."));
			return;
		}
		if (MatchQueueStartUtc.IsEmpty())
		{
			OnMatchmakingFailed.Broadcast(TEXT("파티 리더가 아직 매칭을 시작하지 않았습니다."));
			return;
		}
		bJoiningPartyMatchInvite = true;
		LobbyOwnerId = InviteResult.Session.OwningUserId.IsValid() ? InviteResult.Session.OwningUserId->ToString() : FString();
		const FString LocalId = MakeLocalPlayerId();
		FNFMatchQueueEntry PartyQueueEntry;
		PartyQueueEntry.QueueId = ActiveQueueDefinition.QueueId;
		PartyQueueEntry.PartyId = PartyId.IsEmpty() ? LocalId : PartyId;
		PartyQueueEntry.PartyLeaderId = PartyLeaderId.IsEmpty() ? LocalId : PartyLeaderId;
		PartyQueueEntry.PlayerIds.Add(LocalId);
		const int64 QueueStartTimestamp = FCString::Atoi64(*MatchQueueStartUtc);
		const FDateTime QueueStartTime = FDateTime::FromUnixTimestamp(QueueStartTimestamp);
		const double RemainingSeconds = (QueueStartTime + FTimespan::FromSeconds(ActiveQueueDefinition.QueueTimeoutSeconds) - FDateTime::UtcNow()).GetTotalSeconds();
		if (RemainingSeconds <= 0.0)
		{
			bJoiningPartyMatchInvite = false;
			OnMatchmakingFailed.Broadcast(TEXT("파티 매치 초대가 수락되기 전에 대기 시간 3분이 지났습니다."));
			return;
		}
		PartyQueueEntry.QueueJoinTimes.Add(QueueStartTime);
		QueueEntries.Add(MoveTemp(PartyQueueEntry));
		LocalPartyId = PartyId;
		PlayerPartyId.Add(LocalId, PartyId);
		PlayerPartyLeaderId.Add(LocalId, PartyLeaderId);
		PlayerQueueJoinTime.Add(LocalId, QueueStartTime);
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(QueueTimeoutTimerHandle, this, &ThisClass::HandleQueueTimeout, static_cast<float>(RemainingSeconds), false);
		}
		QueueDeadline = QueueStartTime + FTimespan::FromSeconds(ActiveQueueDefinition.QueueTimeoutSeconds);
		SetMatchState(ENFMatchState::Queueing);
		RecalculateQueue();
		if (MatchState == ENFMatchState::ServerPreparing)
		{
			bJoiningPartyMatchInvite = false;
			return;
		}
	}

	JoinSessionCompleteHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleJoinSessionComplete));
	if (!SessionInterface->JoinSession(LocalUserNum, TargetSessionName, InviteResult))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
		OnMatchmakingFailed.Broadcast(TEXT("Steam 초대 로비에 참가하지 못했습니다."));
		if (TargetSessionName == MatchSessionName) CancelMatchmaking(MakeLocalPlayerId());
	}
}

void UNF_MatchmakingSubsystem::PublishCurrentLobbyMemberData(FName SessionName, const FString& QueueStartUtc)
{
	const uint64 LobbyId = GetSteamLobbyId(SessionInterface, SessionName);
	const FString LocalPlayerId = MakeLocalPlayerId();
	const FString PartyId = LocalPartyId.IsEmpty() ? PlayerPartyId.FindRef(LocalPlayerId) : LocalPartyId;
	FString PartyLeaderId = PlayerPartyLeaderId.FindRef(LocalPlayerId);
	if (PartyLeaderId.IsEmpty())
	{
		PartyLeaderId = PendingPartyLeaderId.IsEmpty() ? LocalPlayerId : PendingPartyLeaderId;
	}
	if (SessionName == PartySessionName)
	{
		SetSteamLobbyData(LobbyId, "NF_PARTY_ID", PartyId);
		SetSteamLobbyData(LobbyId, "NF_PARTY_LEADER", PartyLeaderId);
		SetSteamLobbyData(LobbyId, "NF_LOBBY_KIND", TEXT("Party"));
	}
	else if (SessionName == MatchSessionName && bIsLobbyLeader)
	{
		SetSteamLobbyData(LobbyId, "NF_MATCHMAKING", TEXT("1"));
		SetSteamLobbyData(LobbyId, "NF_LOBBY_KIND", TEXT("Match"));
		SetSteamLobbyData(LobbyId, LobbyMatchStateKey, TEXT("LobbyWaiting"));
		SetSteamLobbyData(LobbyId, LobbyQueueIdKey, ActiveQueueDefinition.QueueId.ToString());
	}
	if (LobbyId != 0 && SteamMatchmaking())
	{
		SteamMatchmaking()->SetLobbyMemberData(CSteamID(LobbyId), MemberPartyIdKey, TCHAR_TO_UTF8(*PartyId));
		SteamMatchmaking()->SetLobbyMemberData(CSteamID(LobbyId), MemberPartyLeaderKey, TCHAR_TO_UTF8(*PartyLeaderId));
		if (!QueueStartUtc.IsEmpty())
		{
			SteamMatchmaking()->SetLobbyMemberData(CSteamID(LobbyId), MemberQueueStartKey, TCHAR_TO_UTF8(*QueueStartUtc));
		}
	}
}

void UNF_MatchmakingSubsystem::InvitePartyMembersToMatchLobby()
{
	if (!SessionInterface.IsValid() || LocalPartyId.IsEmpty() || !SessionInterface->GetNamedSession(MatchSessionName))
	{
		return;
	}
	const FPartyRecord* Party = Parties.Find(LocalPartyId);
	if (!Party || Party->LeaderId != MakeLocalPlayerId())
	{
		return;
	}
	IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld());
	const IOnlineIdentityPtr Identity = OnlineSubsystem ? OnlineSubsystem->GetIdentityInterface() : nullptr;
	if (!Identity.IsValid())
	{
		return;
	}
	for (const FString& MemberId : Party->Members)
	{
		if (MemberId == Party->LeaderId)
		{
			continue;
		}
		const FUniqueNetIdPtr FriendId = Identity->CreateUniquePlayerId(MemberId);
		if (FriendId.IsValid())
		{
			SessionInterface->SendSessionInviteToFriend(0, MatchSessionName, *FriendId);
		}
	}
}

void UNF_MatchmakingSubsystem::RefreshPartyRoster()
{
	const uint64 PartyLobbyId = GetSteamLobbyId(SessionInterface, PartySessionName);
	FPartyRecord* Party = Parties.Find(LocalPartyId);
	if (!Party || PartyLobbyId == 0 || !SteamMatchmaking())
	{
		return;
	}
	const TArray<FString> PreviousMembers = Party->Members;
	TArray<FString> CurrentMembers;
	for (int32 MemberIndex = 0; MemberIndex < SteamMatchmaking()->GetNumLobbyMembers(CSteamID(PartyLobbyId)); ++MemberIndex)
	{
		const FString MemberId = LexToString(SteamMatchmaking()->GetLobbyMemberByIndex(CSteamID(PartyLobbyId), MemberIndex).ConvertToUint64());
		CurrentMembers.Add(MemberId);
		PlayerToParty.Add(MemberId, LocalPartyId);
		PlayerPartyId.Add(MemberId, LocalPartyId);
		PlayerPartyLeaderId.Add(MemberId, Party->LeaderId);
	}
	for (const FString& PreviousMember : PreviousMembers)
	{
		if (!CurrentMembers.Contains(PreviousMember) && (MatchState == ENFMatchState::Queueing || MatchState == ENFMatchState::LobbyWaiting))
		{
			const uint64 MatchLobbyId = GetSteamLobbyId(SessionInterface, MatchSessionName);
			if (Party->LeaderId == MakeLocalPlayerId()) SetSteamLobbyData(MatchLobbyId, LobbyPartyCanceledKey, LocalPartyId);
			OnMatchmakingFailed.Broadcast(TEXT("파티원이 이탈하여 파티 전체를 대기열에서 제거했습니다."));
			CancelMatchmaking(MakeLocalPlayerId());
			return;
		}
	}
	if (!CurrentMembers.Contains(Party->LeaderId))
	{
		if (MatchState == ENFMatchState::Queueing || MatchState == ENFMatchState::LobbyWaiting)
		{
			SetSteamLobbyData(GetSteamLobbyId(SessionInterface, MatchSessionName), LobbyPartyCanceledKey, LocalPartyId);
			OnMatchmakingFailed.Broadcast(TEXT("파티 리더가 나가 파티 매칭을 취소했습니다."));
			CancelMatchmaking(MakeLocalPlayerId());
		}
		else
		{
			for (const FString& MemberId : PreviousMembers) PlayerToParty.Remove(MemberId);
			Parties.Remove(LocalPartyId);
			LocalPartyId.Reset();
			if (SessionInterface.IsValid()) SessionInterface->DestroySession(PartySessionName);
		}
		return;
	}
	Party->Members = MoveTemp(CurrentMembers);
}

void UNF_MatchmakingSubsystem::RefreshLobbyPlayerCount()
{
	if (!SessionInterface.IsValid())
	{
		return;
	}
	const FNamedOnlineSession* NamedSession = SessionInterface->GetNamedSession(MatchSessionName);
	if (!NamedSession)
	{
		return;
	}

	const uint64 LobbyId = GetSteamLobbyId(SessionInterface, MatchSessionName);
	if (LobbyId == 0 || !SteamMatchmaking())
	{
		return;
	}
	RefreshPartyRoster();
	if (MatchState == ENFMatchState::Canceled) return;
	if (MatchState == ENFMatchState::Queueing || MatchState == ENFMatchState::LobbyWaiting)
	{
		const int64 RemainingSeconds = (QueueDeadline - FDateTime::UtcNow()).GetTotalSeconds();
		if (RemainingSeconds <= 0)
		{
			OnMatchmakingFailed.Broadcast(TEXT("이 플레이어의 대기 시작 시각 기준 3분이 지나 매칭을 취소했습니다."));
			CancelMatchmaking(MakeLocalPlayerId());
			return;
		}
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(QueueTimeoutTimerHandle, this, &ThisClass::HandleQueueTimeout, static_cast<float>(RemainingSeconds), false);
		}
	}
	const FString CurrentLobbyOwnerId = LexToString(SteamMatchmaking()->GetLobbyOwner(CSteamID(LobbyId)).ConvertToUint64());
	if (!LobbyOwnerId.IsEmpty() && CurrentLobbyOwnerId != LobbyOwnerId && (MatchState == ENFMatchState::Queueing || MatchState == ENFMatchState::LobbyWaiting))
	{
		OnMatchmakingFailed.Broadcast(TEXT("매칭 Lobby 소유자가 이탈하여 대기열을 취소했습니다."));
		CancelMatchmaking(MakeLocalPlayerId());
		return;
	}
	TSet<FString> CurrentMatchMembers;
	for (int32 MemberIndex = 0; MemberIndex < SteamMatchmaking()->GetNumLobbyMembers(CSteamID(LobbyId)); ++MemberIndex)
	{
		CurrentMatchMembers.Add(LexToString(SteamMatchmaking()->GetLobbyMemberByIndex(CSteamID(LobbyId), MemberIndex).ConvertToUint64()));
	}
	for (const FString& PreviousMember : PreviousMatchMembers)
	{
		if (!CurrentMatchMembers.Contains(PreviousMember))
		{
			if (const FString* DepartingPartyId = MatchMemberPartyIds.Find(PreviousMember))
			{
				if (bIsLobbyLeader) SetSteamLobbyData(LobbyId, LobbyPartyCanceledKey, *DepartingPartyId);
			}
		}
	}
	PreviousMatchMembers = MoveTemp(CurrentMatchMembers);
	QueuedPlayerCount = FMath::Clamp(SteamMatchmaking()->GetNumLobbyMembers(CSteamID(LobbyId)), 0, ActiveQueueDefinition.PlayerCount);
	OnQueueCountChanged.Broadcast(QueuedPlayerCount, ActiveQueueDefinition.PlayerCount);
	MatchMemberPartyIds.Reset();
	if (LobbyId != 0 && SteamMatchmaking())
	{
		for (int32 MemberIndex = 0; MemberIndex < SteamMatchmaking()->GetNumLobbyMembers(CSteamID(LobbyId)); ++MemberIndex)
		{
			const CSteamID SteamMemberId = SteamMatchmaking()->GetLobbyMemberByIndex(CSteamID(LobbyId), MemberIndex);
			const FString MemberId = LexToString(SteamMemberId.ConvertToUint64());
			const FString PartyId = GetSteamLobbyMemberData(LobbyId, SteamMemberId.ConvertToUint64(), MemberPartyIdKey);
			if (!PartyId.IsEmpty()) MatchMemberPartyIds.Add(MemberId, PartyId);
			const FString MemberCancelPartyId = GetSteamLobbyMemberData(LobbyId, SteamMemberId.ConvertToUint64(), MemberCanceledPartyKey);
			if (bIsLobbyLeader && !MemberCancelPartyId.IsEmpty()) SetSteamLobbyData(LobbyId, LobbyPartyCanceledKey, MemberCancelPartyId);
		}
	}
	const FString CanceledPartyId = GetSteamLobbyData(LobbyId, LobbyPartyCanceledKey);
	if (!CanceledPartyId.IsEmpty() && CanceledPartyId == LocalPartyId && (MatchState == ENFMatchState::Queueing || MatchState == ENFMatchState::LobbyWaiting))
	{
		OnMatchmakingFailed.Broadcast(TEXT("파티원이 대기열을 떠나 파티 전체를 취소했습니다."));
		CancelMatchmaking(MakeLocalPlayerId());
		return;
	}
	if (LobbyId != 0 && SteamMatchmaking())
	{
		for (int32 MemberIndex = 0; MemberIndex < SteamMatchmaking()->GetNumLobbyMembers(CSteamID(LobbyId)); ++MemberIndex)
		{
			const CSteamID SteamMemberId = SteamMatchmaking()->GetLobbyMemberByIndex(CSteamID(LobbyId), MemberIndex);
			const FString MemberCanceledPartyId = GetSteamLobbyMemberData(LobbyId, SteamMemberId.ConvertToUint64(), MemberCanceledPartyKey);
			if (!MemberCanceledPartyId.IsEmpty() && MemberCanceledPartyId == LocalPartyId && (MatchState == ENFMatchState::Queueing || MatchState == ENFMatchState::LobbyWaiting))
			{
				if (bIsLobbyLeader) SetSteamLobbyData(LobbyId, LobbyPartyCanceledKey, MemberCanceledPartyId);
				OnMatchmakingFailed.Broadcast(TEXT("파티원이 대기열을 떠나 파티 전체를 취소했습니다."));
				CancelMatchmaking(MakeLocalPlayerId());
				return;
			}
		}
	}
	if (QueuedPlayerCount >= ActiveQueueDefinition.PlayerCount)
	{
		if (bIsLobbyLeader && !bMatchLobbyAdmissionClosed)
		{
			const bool bSteamLobbyClosed = SteamMatchmaking()->SetLobbyJoinable(CSteamID(LobbyId), false);
			if (!bSteamLobbyClosed)
			{
				UE_LOG(LogProject_NF, Warning, TEXT("Steam refused to close the full match lobby to new members."));
			}
			FOnlineSessionSettings ClosedSettings = NamedSession->SessionSettings;
			ClosedSettings.bAllowJoinInProgress = false;
			ClosedSettings.bAllowJoinViaPresence = false;
			ClosedSettings.bShouldAdvertise = false;
			const bool bSessionUpdateStarted = SessionInterface->UpdateSession(MatchSessionName, ClosedSettings, true);
			if (!bSessionUpdateStarted)
			{
				UE_LOG(LogProject_NF, Warning, TEXT("Failed to update the full match session to reject new members."));
			}
			bMatchLobbyAdmissionClosed = bSteamLobbyClosed || bSessionUpdateStarted;
		}
		if (!bTeamsAssigned && bIsLobbyLeader)
		{
			TArray<FNFQueuedPlayer> MatchRoster;
			const uint64 CurrentLobbyId = GetSteamLobbyId(SessionInterface, MatchSessionName);
			if (CurrentLobbyId == 0 || !SteamMatchmaking())
			{
				return;
			}
			for (int32 MemberIndex = 0; MemberIndex < SteamMatchmaking()->GetNumLobbyMembers(CSteamID(CurrentLobbyId)); ++MemberIndex)
			{
				const CSteamID SteamMemberId = SteamMatchmaking()->GetLobbyMemberByIndex(CSteamID(CurrentLobbyId), MemberIndex);
				const FString PlayerId = LexToString(SteamMemberId.ConvertToUint64());
				const FString PartyId = GetSteamLobbyMemberData(CurrentLobbyId, SteamMemberId.ConvertToUint64(), MemberPartyIdKey);
				const FString PartyLeaderId = GetSteamLobbyMemberData(CurrentLobbyId, SteamMemberId.ConvertToUint64(), MemberPartyLeaderKey);
				const FString QueueStartUtc = GetSteamLobbyMemberData(CurrentLobbyId, SteamMemberId.ConvertToUint64(), MemberQueueStartKey);
				if (PlayerId.IsEmpty() || PartyId.IsEmpty() || PartyLeaderId.IsEmpty() || QueueStartUtc.IsEmpty())
				{
					return;
				}
				const int64 QueueStartTimestamp = FCString::Atoi64(*QueueStartUtc);
				if (QueueStartTimestamp <= 0 || FDateTime::FromUnixTimestamp(QueueStartTimestamp) + FTimespan::FromSeconds(ActiveQueueDefinition.QueueTimeoutSeconds) <= FDateTime::UtcNow())
				{
					OnMatchmakingFailed.Broadcast(TEXT("참가자별 3분 대기 시간이 만료되어 매칭을 취소했습니다."));
					CancelMatchmaking(MakeLocalPlayerId());
					return;
				}
				FNFQueuedPlayer& Player = MatchRoster.AddDefaulted_GetRef();
				Player.PlayerId = PlayerId;
				Player.PartyId = PartyId;
				Player.PartyLeaderId = PartyLeaderId;
			}
			TArray<FNFMatchTeamAssignment> Teams;
			if (!NF_AssignMatchTeams(ActiveQueueDefinition, MatchRoster, Teams))
			{
				OnMatchmakingFailed.Broadcast(TEXT("Steam Lobby의 8인 명단 또는 파티 정보가 유효하지 않아 매칭을 취소했습니다."));
				CancelMatchmaking(MakeLocalPlayerId());
				return;
			}
			bTeamsAssigned = true;
			if (!SetSteamLobbyData(CurrentLobbyId, LobbyMatchTeamsKey, SerializeTeams(Teams)) || !SetSteamLobbyData(CurrentLobbyId, LobbyMatchStateKey, TEXT("ServerPreparing")))
			{
				bTeamsAssigned = false;
				OnMatchmakingFailed.Broadcast(TEXT("매치 팀 편성 결과를 Steam Lobby에 게시하지 못했습니다."));
				CancelMatchmaking(MakeLocalPlayerId());
				return;
			}
			PublishMatchTeamsAssigned(Teams);
		}
		if (bIsLobbyLeader)
		{
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().ClearTimer(QueueTimeoutTimerHandle);
				World->GetTimerManager().ClearTimer(LobbyRefreshTimerHandle);
			}
			SetMatchState(ENFMatchState::ServerPreparing);
		}
		else if (!bTeamsAssigned && GetSteamLobbyData(LobbyId, LobbyMatchStateKey) == TEXT("ServerPreparing"))
		{
			TArray<FNFMatchTeamAssignment> PublishedTeams;
			if (!DeserializeTeams(GetSteamLobbyData(LobbyId, LobbyMatchTeamsKey), ActiveQueueDefinition, PublishedTeams))
			{
				OnMatchmakingFailed.Broadcast(TEXT("매치 Lobby의 팀 편성 정보가 없거나 유효하지 않습니다."));
				CancelMatchmaking(MakeLocalPlayerId());
				return;
			}
			bTeamsAssigned = true;
			PublishMatchTeamsAssigned(PublishedTeams);
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().ClearTimer(QueueTimeoutTimerHandle);
				World->GetTimerManager().ClearTimer(LobbyRefreshTimerHandle);
			}
			SetMatchState(ENFMatchState::ServerPreparing);
		}
	}
	else if (MatchState == ENFMatchState::Queueing)
	{
		SetMatchState(ENFMatchState::LobbyWaiting);
	}
	else if (MatchState == ENFMatchState::ServerPreparing && !bIsLobbyLeader)
	{
		TArray<FNFMatchTeamAssignment> PublishedTeams;
		if (DeserializeTeams(GetSteamLobbyData(GetSteamLobbyId(SessionInterface, MatchSessionName), LobbyMatchTeamsKey), ActiveQueueDefinition, PublishedTeams))
		{
			bTeamsAssigned = true;
			PublishMatchTeamsAssigned(PublishedTeams);
		}
	}
}

void UNF_MatchmakingSubsystem::HandleSessionFailure(const FUniqueNetId& PlayerId, ESessionFailure::Type FailureType)
{
	if (MatchState == ENFMatchState::Queueing || MatchState == ENFMatchState::LobbyWaiting)
	{
		OnMatchmakingFailed.Broadcast(TEXT("Steam Lobby 연결이 끊겨 대기열에서 제거되었습니다."));
		CancelMatchmaking(MakeLocalPlayerId());
	}
}

void UNF_MatchmakingSubsystem::CleanupSession()
{
	if (!SessionInterface.IsValid())
	{
		return;
	}
	if (CreateSessionCompleteHandle.IsValid()) SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
	if (FindSessionsCompleteHandle.IsValid()) SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
	if (JoinSessionCompleteHandle.IsValid()) SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
	if (DestroySessionCompleteHandle.IsValid()) SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
	if (SessionParticipantJoinedHandle.IsValid()) SessionInterface->ClearOnSessionParticipantJoinedDelegate_Handle(SessionParticipantJoinedHandle);
	if (SessionParticipantLeftHandle.IsValid()) SessionInterface->ClearOnSessionParticipantLeftDelegate_Handle(SessionParticipantLeftHandle);
	if (SessionFailureHandle.IsValid()) SessionInterface->ClearOnSessionFailureDelegate_Handle(SessionFailureHandle);
	if (SessionUserInviteAcceptedHandle.IsValid()) SessionInterface->ClearOnSessionUserInviteAcceptedDelegate_Handle(SessionUserInviteAcceptedHandle);
}

void UNF_MatchmakingSubsystem::CancelWholeQueuedParty(const FString& PlayerId)
{
	const FString PartyId = PlayerPartyId.FindRef(PlayerId);
	if (PartyId.IsEmpty())
	{
		QueueEntries.RemoveAll([&PlayerId](const FNFMatchQueueEntry& Entry) { return Entry.PlayerIds.Contains(PlayerId); });
		PlayerPartyId.Remove(PlayerId);
		PlayerPartyLeaderId.Remove(PlayerId);
		PlayerQueueJoinTime.Remove(PlayerId);
		RecalculateQueue();
		return;
	}

	QueueEntries.RemoveAll([&PartyId](const FNFMatchQueueEntry& Entry) { return Entry.PartyId == PartyId; });
	for (auto It = PlayerPartyId.CreateIterator(); It; ++It)
	{
		if (It.Value() == PartyId)
		{
			PlayerPartyLeaderId.Remove(It.Key());
			PlayerQueueJoinTime.Remove(It.Key());
			It.RemoveCurrent();
		}
	}
	RecalculateQueue();
}

FString UNF_MatchmakingSubsystem::MakeLocalPlayerId() const
{
	if (IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld()))
	{
		if (IOnlineIdentityPtr Identity = OnlineSubsystem->GetIdentityInterface())
		{
			if (FUniqueNetIdPtr UserId = Identity->GetUniquePlayerId(0))
			{
				return UserId->ToString();
			}
		}
	}
	return FString();
}
