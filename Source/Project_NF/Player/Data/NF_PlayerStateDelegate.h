#pragma once

#include "CoreMinimal.h"
#include "NF_PlayerStateDelegate.generated.h"

enum class ENF_PlayerInteractTypeEnum : uint8;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayerOpenShop, ENF_PlayerInteractTypeEnum);

