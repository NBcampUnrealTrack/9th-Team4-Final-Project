#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NF_SurvivalGEDataAsset.generated.h"

class UGameplayEffect;

UCLASS(BlueprintType)
class PROJECT_NF_API UNF_SurvivalGEDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay Effects|Survival")
	TSubclassOf<UGameplayEffect> ReviveHealthEffectClass;
};
