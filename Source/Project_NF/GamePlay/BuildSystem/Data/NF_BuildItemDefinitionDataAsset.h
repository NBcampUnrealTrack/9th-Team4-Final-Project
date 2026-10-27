#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NF_BuildItemDefinitionDataAsset.generated.h"

/** @Owner: 황준현 | 2026-10-26. 상점 식별자와 표시 이름만 소유한다. */
UCLASS(BlueprintType)
class PROJECT_NF_API UNF_BuildItemDefinitionDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Build")
	FName ItemId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Build")
	FText DisplayName;
};
