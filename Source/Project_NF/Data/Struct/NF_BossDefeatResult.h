#pragma once

#include "CoreMinimal.h"
#include "NF_BossDefeatResult.generated.h"

class ANF_BossBase;
class ANF_PlayerState;

USTRUCT(BlueprintType)
struct FNF_BossDamageContribution
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<ANF_PlayerState> PlayerState = nullptr;

	// 방어력, 피해 감소, 오버킬을 제외한 실제 체력 피해량
	UPROPERTY(BlueprintReadOnly)
	float EffectiveDamage = 0.0f;
};

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
	
	//플레이어별 기여도 구조체 배열
	UPROPERTY(BlueprintReadOnly)
	TArray<FNF_BossDamageContribution> Contributions;
	
};