#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "NF_BuffDisplayDataAsset.generated.h"

class UTexture2D;
class UGameplayEffect;

UCLASS(BlueprintType)
class PROJECT_NF_API UNF_BuffDisplayDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Buff Display")
	FGameplayTag BuffTag;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Buff Display")
	TSubclassOf<UGameplayEffect> BuffEffectClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Buff Display")
	TArray<TSoftObjectPtr<UTexture2D>> Icons;
};
