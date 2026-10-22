#pragma once

#include "CoreMinimal.h"
#include "Data/Enum/NF_MatchPhase.h"
#include "Data/Struct/NF_MatchResult.h"
#include "NF_MatchState.generated.h"

USTRUCT(BlueprintType)
struct FNF_MatchState
{
	GENERATED_BODY()

	// 초기화는 Waiting으로
	UPROPERTY(BlueprintReadOnly)
	ENF_MatchPhase Phase = ENF_MatchPhase::Waiting;

	// 해당 페이즈가 제한시간이 존재하는가
	UPROPERTY(BlueprintReadOnly)
	bool bHasTimeLimit = false;
	
	// 현재 Phase가 종료되는 서버 시각.
	UPROPERTY(BlueprintReadOnly)
	double PhaseEndServerTime = 0.0;
	
	UPROPERTY(BlueprintReadOnly)
	FNF_MatchResult Result;
};
