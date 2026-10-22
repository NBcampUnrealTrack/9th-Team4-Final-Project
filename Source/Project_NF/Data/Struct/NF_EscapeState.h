#pragma once

#include "CoreMinimal.h"
#include "NF_EscapeState.generated.h"

class ANF_PlayerState;

UENUM(BlueprintType)
enum class ENF_EscapeState : uint8
{
	Idle                    UMETA(DisplayName = "대기"), // 대기
	CountingWithTeammate    UMETA(DisplayName = "팀 탈출 진행"), //10초
	PausedForTeammate       UMETA(DisplayName = "팀원 대기"), //진행상태보존하며 일시정지
	CountingSolo            UMETA(DisplayName = "단독 탈출 진행") //20초
};

USTRUCT(BlueprintType)
struct FNF_EscapeProgress
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	bool bInProgress = false;

	UPROPERTY(BlueprintReadOnly)
	bool bPaused = false;

	UPROPERTY(BlueprintReadOnly)
	ENF_EscapeState State = ENF_EscapeState::Idle;

	UPROPERTY(BlueprintReadOnly)
	int32 TeamId = INDEX_NONE;

	// UI 진행률 계산용 전체 시간
	UPROPERTY(BlueprintReadOnly)
	float TotalDuration = 0.0f;

	// 일시정지 상태일 때 보존된 남은 시간
	UPROPERTY(BlueprintReadOnly)
	float RemainingTime = 0.0f;

	// 진행 중일 때 클라이언트가 남은 시간을 계산할 기준
	UPROPERTY(BlueprintReadOnly)
	double EndServerTime = 0.0;
};

enum class ENF_EscapeValidationState : uint8
{
	Invalid,
	TeamReady,
	WaitingForTeammate,
	SoloReady
};

struct FNF_EscapeValidationResult
{
	ENF_EscapeValidationState State = ENF_EscapeValidationState::Invalid;

	ANF_PlayerState* Holder = nullptr;
	ANF_PlayerState* Teammate = nullptr;
};
