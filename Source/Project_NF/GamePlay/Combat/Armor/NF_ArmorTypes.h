#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "NF_ArmorTypes.generated.h"

UENUM(BlueprintType)
enum class ENF_ArmorSlot : uint8
{
	None,
	HeadArmor,
	BodyArmor,
	LegArmor,
	Unknown
};

// Must match WeaponProjectile in DefaultEngine.ini. Reserved for NF_WeaponProjectile only.
constexpr ECollisionChannel NF_WeaponProjectileChannel = ECC_GameTraceChannel2;
