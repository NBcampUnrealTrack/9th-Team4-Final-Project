#pragma once

#include "CoreMinimal.h"
#include "NF_KeyState.generated.h"

// @owner : 조민웅 | 2026-10-22
// 목적: 서버가 관리하는 열쇠의 현재 존재 형태.
UENUM(BlueprintType)
enum class ENF_KeyState : uint8
{
	Unassigned,
	Held,
	Dropped,
	Removed
};