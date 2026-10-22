#pragma once

#include "CoreMinimal.h"
#include "NF_MatchResult.generated.h"

// @owner : 조민웅 | 2026-10-21
UENUM(BlueprintType)
enum class ENF_MatchOutcome : uint8
{
	None,
	Victory,
	Aborted
};

// @owner : 조민웅 | 2026-10-21
UENUM(BlueprintType)
enum class ENF_MatchEndReason : uint8
{
	None,
	EscapeSucceeded,
	WaitingPlayersTimeout,
	WaitingTeamsTimeout,
	WaitingInitializationTimeout,
	DepartureLimit,
	RuntimeFailure
};

// @owner : 조민웅 | 2026-10-21
// 목적: 서버에서 확정하여 클라이언트에 제공하는 최종 매치 결과.
USTRUCT(BlueprintType)
struct FNF_MatchResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	ENF_MatchOutcome Outcome = ENF_MatchOutcome::None;

	UPROPERTY(BlueprintReadOnly)
	ENF_MatchEndReason EndReason = ENF_MatchEndReason::None;

	UPROPERTY(BlueprintReadOnly)
	int32 WinningTeamId = INDEX_NONE;

	// 오류 안내와 결과 표시용 설명.
	UPROPERTY(BlueprintReadOnly)
	FString Message;
};