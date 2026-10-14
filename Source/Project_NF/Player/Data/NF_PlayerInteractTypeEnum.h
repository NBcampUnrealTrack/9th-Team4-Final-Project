#pragma once

#include "CoreMinimal.h"
#include "NF_PlayerInteractTypeEnum.generated.h"

UENUM(BlueprintType)
enum class ENF_PlayerInteractTypeEnum : uint8
{
	None UMETA(DisplayName = "None"),
	SkillUsable UMETA(DisplayName = "BattleAvailable"),
	InteractingBuild UMETA(DisplayName = "InteractingBuild"),
};
