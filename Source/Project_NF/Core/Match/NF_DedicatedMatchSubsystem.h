#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Core/Match/NF_MatchRules.h"
#include "NF_DedicatedMatchSubsystem.generated.h"

class APlayerController;
class ANF_PlayerState;

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

private:
	bool ValidateTeamAssignments(const TArray<FNFMatchTeamAssignment>& Teams, FString& OutError) const;
	FString MakeAssignmentSignature(const TArray<FNFMatchTeamAssignment>& Teams) const;
	void TryApplyTeamAssignments();
	void ScheduleDedicatedServerExit();
	void ExitDedicatedServer();
	void HandleSessionDestroyed(FName SessionName, bool bWasSuccessful);

	TMap<FString, TWeakObjectPtr<ANF_PlayerState>> SubmittedPlayerStates;
	TSet<FString> SubmittedPlayerIds;
	TArray<FNFMatchTeamAssignment> CurrentTeams;
	FString ExpectedAssignmentSignature;
	FTimerHandle DedicatedServerExitTimerHandle;
	FDelegateHandle DestroySessionCompleteHandle;
	bool bTeamAssignmentsApplied = false;
	bool bMatchAbortHandled = false;
};
