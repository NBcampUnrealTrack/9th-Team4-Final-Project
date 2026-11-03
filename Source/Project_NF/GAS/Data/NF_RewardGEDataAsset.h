#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NF_RewardGEDataAsset.generated.h"

class UGameplayEffect;

UCLASS(BlueprintType)
class PROJECT_NF_API UNF_RewardGEDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay Effects|Reward")
	TSubclassOf<UGameplayEffect> KeyHolderRewardEffectClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay Effects|Reward")
	TSubclassOf<UGameplayEffect> BossContributionRecoveryEffectClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay Effects|Reward")
	TSubclassOf<UGameplayEffect> LowestContributionPenaltyEffectClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay Effects|Reward")
	TSubclassOf<UGameplayEffect> ShieldClampEffectClass;
};
