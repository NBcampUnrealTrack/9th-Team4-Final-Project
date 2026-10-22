#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Core/Match/NF_MatchRules.h"
#include "Data/Struct/NF_MatchResult.h"
#include "NF_DedicatedMatchSubsystem.generated.h"

class APlayerController;
class ANF_PlayerState;

/** One authoritative server-side PlayerState-to-team mapping, broadcast after assignment is applied. */
struct FNFServerTeamIdAssignment
{
	TWeakObjectPtr<ANF_PlayerState> PlayerState;
	int32 TeamId = INDEX_NONE;
};

DECLARE_MULTICAST_DELEGATE_OneParam(
	FNFOnServerTeamIdsAssigned,
	const TArray<FNFServerTeamIdAssignment>&);

/** Server-side coordinator for validating lobby rosters and applying team IDs. */
UCLASS()
class PROJECT_NF_API UNF_DedicatedMatchSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;
	bool SubmitTeamAssignments(APlayerController* Submitter, const FString& ClaimedPlayerId,
		const TArray<FNFMatchTeamAssignment>& Teams);
	void HandleMatchAbort(const FString& Reason);
	void HandleMatchCompleted(const FNF_MatchResult& Result);

	/** Broadcast once on the server after every PlayerState has its final replicated TeamId. */
	FNFOnServerTeamIdsAssigned OnServerTeamIdsAssigned;

private:
	bool ValidateTeamAssignments(const TArray<FNFMatchTeamAssignment>& Teams, FString& OutError) const;
	FString MakeAssignmentSignature(const TArray<FNFMatchTeamAssignment>& Teams) const;
	void TryApplyTeamAssignments();
	void ScheduleDedicatedServerExit();
	void ExitDedicatedServer();
	void HandleSessionDestroyed(FName SessionName, bool bWasSuccessful);
	void BeginSessionCleanup();
	void HandleSessionCleanupTimeout();

	TMap<FString, TWeakObjectPtr<ANF_PlayerState>> SubmittedPlayerStates;
	TSet<FString> SubmittedPlayerIds;
	TArray<FNFMatchTeamAssignment> CurrentTeams;
	FString ExpectedAssignmentSignature;
	FTimerHandle DedicatedServerExitTimerHandle;
	FTimerHandle SessionCleanupTimerHandle;
	FNF_MatchResult CompletedResult;
	bool bMatchCompletedHandled = false;
	FDelegateHandle DestroySessionCompleteHandle;
	bool bTeamAssignmentsApplied = false;
	bool bMatchAbortHandled = false;
};
