#pragma once

#include "CoreMinimal.h"
#include "NF_WeaponProjectileHitType.generated.h"

// ProjectileHit Cue의 RawMagnitude로 전달하는 명중 대상 분류.
UENUM(BlueprintType)
enum class ENF_WeaponProjectileHitType : uint8
{
	Environment = 0,
	Boss = 1,
	Monster = 2,
	Player = 3
};
