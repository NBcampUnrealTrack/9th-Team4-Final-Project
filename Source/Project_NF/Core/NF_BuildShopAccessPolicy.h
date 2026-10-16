#pragma once

#include "CoreMinimal.h"
#include "Data/Enum/NF_BuildShopType.h"
#include "Data/Enum/NF_MatchPhase.h"


//매치 페이즈와 상점 종류만 검사
struct PROJECT_NF_API FNF_BuildShopAccessPolicy
{
	static bool IsShopAllowed(
		ENF_MatchPhase MatchPhase,
		ENF_BuildShopType ShopKind);
};