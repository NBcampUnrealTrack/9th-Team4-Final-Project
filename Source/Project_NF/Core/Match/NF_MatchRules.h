// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NF_MatchRules.generated.h"

/** Runtime stages for an eight-player match. */
UENUM(BlueprintType)
enum class ENFMatchState : uint8
{
	Queueing        UMETA(DisplayName = "Queueing"),
	LobbyWaiting    UMETA(DisplayName = "Lobby Waiting"),
	ServerPreparing UMETA(DisplayName = "Server Preparing"),
	ServerConnecting UMETA(DisplayName = "Server Connecting"),
	InProgress      UMETA(DisplayName = "In Progress"),
	Ended           UMETA(DisplayName = "Ended"),
	Canceled        UMETA(DisplayName = "Canceled")
};

/** Immutable limits and policies for the initial matchmaking implementation. */
namespace NF_MatchRules
{
	inline constexpr int32 PlayerCount = 8;
	inline constexpr int32 TeamCount = 4;
	inline constexpr int32 PlayersPerTeam = 2;
	inline constexpr int32 MaxPartySize = 2;
	inline constexpr int32 QueueTimeoutSeconds = 180;

	/** A queued party is canceled in full if any member leaves before the match starts. */
	inline constexpr bool bCancelWholePartyWhenMemberLeaves = true;

	/** Parties are assigned before solo players; party members must occupy the same team. */
	inline constexpr bool bAssignPartiesBeforeSoloPlayers = true;

	/** Initial policy: fill each team to two players, preserving each party as a unit. */
	inline constexpr bool bKeepPartyMembersOnSameTeam = true;
}

/** A player and their party metadata as consumed by the team assignment policy. */
USTRUCT(BlueprintType)
struct FNFQueuedPlayer
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString PlayerId;

	UPROPERTY(BlueprintReadWrite)
	FString PartyId;

	UPROPERTY(BlueprintReadWrite)
	FString PartyLeaderId;
};

/** Result of packing eight queued players into four two-player teams. */
USTRUCT(BlueprintType)
struct FNFMatchTeamAssignment
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TArray<FNFQueuedPlayer> Players;
};

/** Deterministic party-first team packing. Returns false for invalid or unfillable rosters. */
inline bool NF_AssignMatchTeams(const TArray<FNFQueuedPlayer>& QueuedPlayers, TArray<FNFMatchTeamAssignment>& OutTeams)
{
	OutTeams.Reset();
	if (QueuedPlayers.Num() != NF_MatchRules::PlayerCount)
	{
		return false;
	}

	TSet<FString> UniquePlayerIds;
	TMap<FString, TArray<FNFQueuedPlayer>> Parties;
	for (const FNFQueuedPlayer& Player : QueuedPlayers)
	{
		if (Player.PlayerId.IsEmpty() || Player.PartyId.IsEmpty() || UniquePlayerIds.Contains(Player.PlayerId))
		{
			return false;
		}
		UniquePlayerIds.Add(Player.PlayerId);
		Parties.FindOrAdd(Player.PartyId).Add(Player);
	}

	for (const TPair<FString, TArray<FNFQueuedPlayer>>& Party : Parties)
	{
		if (Party.Key.IsEmpty() || Party.Value.Num() > NF_MatchRules::MaxPartySize)
		{
			return false;
		}
		const FString& LeaderId = Party.Value[0].PartyLeaderId;
		if (LeaderId.IsEmpty())
		{
			return false;
		}
		bool bLeaderIsMember = false;
		for (const FNFQueuedPlayer& Player : Party.Value)
		{
			if (Player.PartyLeaderId != LeaderId) return false;
			bLeaderIsMember |= Player.PlayerId == LeaderId;
		}
		if (!bLeaderIsMember) return false;
	}

	OutTeams.SetNum(NF_MatchRules::TeamCount);
	for (const TPair<FString, TArray<FNFQueuedPlayer>>& Party : Parties)
	{
		if (Party.Value.Num() != NF_MatchRules::MaxPartySize)
		{
			continue;
		}

		int32 TeamIndex = INDEX_NONE;
		for (int32 Index = 0; Index < OutTeams.Num(); ++Index)
		{
			if (OutTeams[Index].Players.IsEmpty())
			{
				TeamIndex = Index;
				break;
			}
		}
		if (TeamIndex == INDEX_NONE)
		{
			OutTeams.Reset();
			return false;
		}
		OutTeams[TeamIndex].Players.Append(Party.Value);
	}

	int32 SoloIndex = 0;
	for (const TPair<FString, TArray<FNFQueuedPlayer>>& Party : Parties)
	{
		if (Party.Value.Num() != 1)
		{
			continue;
		}

		while (SoloIndex < OutTeams.Num() && OutTeams[SoloIndex].Players.Num() >= NF_MatchRules::PlayersPerTeam)
		{
			++SoloIndex;
		}
		if (SoloIndex >= OutTeams.Num())
		{
			OutTeams.Reset();
			return false;
		}
		OutTeams[SoloIndex].Players.Add(Party.Value[0]);
	}

	for (const FNFMatchTeamAssignment& Team : OutTeams)
	{
		if (Team.Players.Num() != NF_MatchRules::PlayersPerTeam)
		{
			OutTeams.Reset();
			return false;
		}
	}
	return true;
}
