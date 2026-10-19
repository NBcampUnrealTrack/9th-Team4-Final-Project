#pragma once

#include "CoreMinimal.h"
#include "NF_BossDefeatResult.generated.h"

class ANF_BossBase;
class ANF_PlayerState;

USTRUCT(BlueprintType)
struct FNF_BossDefeatResult
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<ANF_BossBase> DefeatedBoss = nullptr;

	// 막타 플레이어의 PlayerState
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<ANF_PlayerState> LastHitPlayerState = nullptr;


	UPROPERTY(BlueprintReadOnly)
	int32 LastHitPlayerID = INDEX_NONE;
	
};