#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NF_PlayerLifeDataAsset.generated.h"

class UGameplayEffect;

UCLASS(BlueprintType)
class PROJECT_NF_API UNF_PlayerLifeDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	// 기절 지속시간 임시값 (30초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Life|Downed", meta = (ClampMin = "0.1"))
	float DownedDurationSeconds = 30.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Life|Revive")
	TSubclassOf<UGameplayEffect> ReviveHealthEffectClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Life|Revive", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float ReviveHealthRate = 0.3f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Life|Revive", meta = (ClampMin = "1.0"))
	float MaxReviveDistance = 50.0f;
};
