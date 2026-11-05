#pragma once

#include "CoreMinimal.h"
#include "NF_CustomVisualMeshType.generated.h"

UENUM(BlueprintType)
enum class ENF_CustomVisualMeshType : uint8
{
	None UMETA(DisplayName = "None"),
	Boy_01 UMETA(DisplayName = "Boy_01"),
	Boy_02 UMETA(DisplayName = "Boy_02"),
	Boy_03 UMETA(DisplayName = "Boy_03"),
	Boy_04 UMETA(DisplayName = "Boy_04"),
	Girl_01 UMETA(DisplayName = "Girl_01"),
	Girl_02 UMETA(DisplayName = "Girl_02"),
	Girl_03 UMETA(DisplayName = "Girl_03"),
	Girl_04 UMETA(DisplayName = "Girl_04"),
};
