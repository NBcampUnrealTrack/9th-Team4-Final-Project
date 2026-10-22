// @owner : 조민웅 | 2026-10-21
// 최초 지급 처리의 상태. 현재 열쇠 소유 상태와는 구분한다.
#pragma once

#include "CoreMinimal.h"
#include "NF_KeyInitState.generated.h"

UENUM(BlueprintType)
enum class ENF_KeyInitState : uint8
{
	NotStarted,
	Retrying,
	Completed,
	Failed,
	Cancelled
};