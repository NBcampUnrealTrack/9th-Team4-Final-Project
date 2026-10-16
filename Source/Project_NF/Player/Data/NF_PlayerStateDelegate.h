#pragma once

#include "CoreMinimal.h"

enum class ENF_PlayerInteractTypeEnum : uint8;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayerOpenShop, ENF_PlayerInteractTypeEnum);

