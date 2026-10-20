#pragma once

#include "CoreMinimal.h"
#include "NF_BuildShopType.generated.h"

/**
 * @owner 민웅: Core | 2026-10-16
 * 매치 규칙에서 사용하는 빌드 상점 분류.
 */
UENUM(BlueprintType)
enum class ENF_BuildShopType : uint8
{
	None UMETA(DisplayName = "None"),
	TeamBase UMETA(DisplayName = "Team Base"),
	Field UMETA(DisplayName = "Field")
};
