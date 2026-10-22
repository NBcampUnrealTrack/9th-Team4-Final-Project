#pragma once

#include "CoreMinimal.h"
#include "Data/Enum/NF_KeyState.h"
#include "NF_KeyPublicState.generated.h"

class ANF_PlayerState;

// @owner : 조민웅 | 2026-10-22
// 목적: 클라이언트 UI가 읽을 열쇠의 공개 상태.
USTRUCT(BlueprintType)
struct FNF_KeyPublicState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	ENF_KeyState State = ENF_KeyState::Unassigned;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<ANF_PlayerState> Holder = nullptr;
};