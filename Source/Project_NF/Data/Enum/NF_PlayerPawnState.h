#pragma once

#include "CoreMinimal.h"
#include "NF_PlayerPawnState.generated.h"

// @owner : 조민웅 | 2026-10-27
// 목적: Pawn이 없는 이유를 최초 생성 전, 리스폰 대기, 탈락 상태로 구분한다.
UENUM(BlueprintType)
enum class ENF_PlayerPawnState : uint8
{
	Unknown UMETA(DisplayName = "Unknown"),
	InitialSpawnPending UMETA(DisplayName = "Initial Spawn Pending"),
	Active UMETA(DisplayName = "Active"),
	RespawnPending UMETA(DisplayName = "Respawn Pending"),
	Eliminated UMETA(DisplayName = "Eliminated")
};
