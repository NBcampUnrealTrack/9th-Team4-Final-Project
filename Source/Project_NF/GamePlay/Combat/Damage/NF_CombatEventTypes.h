#pragma once

#include "CoreMinimal.h"
#include "NF_CombatEventTypes.generated.h"

UENUM(BlueprintType)
enum class ENF_AttackType : uint8
{
	None,
	Weapon,
	Melee,
	Skill
};
