#pragma once

#include "CoreMinimal.h"
#include "NF_CustomVisualMeshType.generated.h"

UENUM(BlueprintType)
enum class ENF_CustomVisualMeshType : uint8
{
	None UMETA(DisplayName = "None"),
	Boy_01 UMETA(DisplayName = "Boy_01"),
	Girl_01 UMETA(DisplayName = "Girl_01"),
};
