#pragma once

#include "CoreMinimal.h"
#include "NF_ItemTypes.generated.h"

// @Owner: 황준현 | 2026-10-26. 각 파트 원본과 빌드가 공유하는 항목 분류.

UENUM(BlueprintType)
enum class ENF_ItemType : uint8
{
	Weapon,
	Armor,
	ActiveSkill,
	PassiveSkill
};
