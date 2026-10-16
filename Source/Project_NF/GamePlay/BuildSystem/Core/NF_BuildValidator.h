#pragma once

#include "CoreMinimal.h"
#include "GamePlay/BuildSystem/Core/NF_BuildTypes.h"

class UNF_BuildRules;

/** Pure validation shared by UI previews and authoritative application. */
class PROJECT_NF_API FNF_BuildValidator
{
public:
	static FNF_BuildValidation Validate(
		const UNF_BuildRules* Rules, int32 PlayerLevel, const TArray<FNF_BuildSelection>& Selections);
};
