#pragma once

#include "CoreMinimal.h"
#include "NF_MatchPhase.generated.h"

UENUM(BlueprintType)
enum class ENF_MatchPhase : uint8
{
	Waiting     UMETA(DisplayName = "Waiting"),
	Build       UMETA(DisplayName = "Build"),
	Farming     UMETA(DisplayName = "Farming"),
	Boss        UMETA(DisplayName = "Boss"),
	Escape      UMETA(DisplayName = "Escape"),
	Finished    UMETA(DisplayName = "Finished")
};


