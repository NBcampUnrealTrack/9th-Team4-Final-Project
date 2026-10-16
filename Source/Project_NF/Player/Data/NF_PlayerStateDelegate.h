#pragma once

#include "CoreMinimal.h"
#include "GamePlay/BuildSystem/Core/NF_BuildTypes.h"
#include "NF_PlayerStateDelegate.generated.h"

enum class ENF_PlayerInteractTypeEnum : uint8;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayerOpenShop, ENF_PlayerInteractTypeEnum);

// @Owner: 황준현 | 2026-10-16
// 빌드 상점 사용 시작·종료 상태를 전달한다.
UDELEGATE()
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNF_OnBuildShopChanged,ENF_BuildShopUIState, SessionState);