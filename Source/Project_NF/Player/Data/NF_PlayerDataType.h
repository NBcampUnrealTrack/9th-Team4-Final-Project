#pragma once

#include "CoreMinimal.h"
#include "NF_PlayerDataType.generated.h"

UENUM(BlueprintType)
enum class ENF_PlayerDataType : uint8
{
	None UMETA(DisplayName = "None"),
	NF_Player UMETA(DisplayName = "NF_Player"),
	NF_PlayerState UMETA(DisplayName = "NF_PlayerState"),
	NF_PlayerDataAsset UMETA(DisplayName = "NF_PlayerDataAsset")
};
