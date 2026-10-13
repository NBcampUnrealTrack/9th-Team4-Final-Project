#pragma once

#include "CoreMinimal.h"
#include "NF_PlayerStateType.generated.h"

UENUM(BlueprintType)
enum class ENF_PlayerStateType : uint8
{
	None UMETA(DisplayName = "None"),
	SkillUsable UMETA(DisplayName = "BattleAvailable"),
	InteractingBuild UMETA(DisplayName = "InteractingBuild"),
};
