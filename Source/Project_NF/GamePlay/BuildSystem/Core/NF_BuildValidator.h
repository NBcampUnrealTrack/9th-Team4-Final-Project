#pragma once

#include "CoreMinimal.h"
#include "GamePlay/BuildSystem/Core/NF_BuildTypes.h"
#include "GamePlay/BuildSystem/Core/NF_BuildItemSource.h"

class UNF_BuildRules;

/** Pure validation shared by UI previews and authoritative application. */
class PROJECT_NF_API FNF_BuildValidator
{
public:
	static FNF_BuildValidation Validate(
		const UNF_BuildRules* Rules,
		int32 CostLimit,
		const TArray<FName>& AllowedItemIds,
		const TArray<FNF_BuildSelection>& Selections,
		TMap<FName, FNF_BuildItemData>* OutSelectedData = nullptr);
};
