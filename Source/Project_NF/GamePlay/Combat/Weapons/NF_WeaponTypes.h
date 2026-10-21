#pragma once

#include "CoreMinimal.h"
#include "NF_WeaponTypes.generated.h"

UENUM(BlueprintType)
enum class ENF_WeaponType : uint8
{
	None,
	AR		UMETA(DisplayName = "Assault Rifle"),
	Pistol	UMETA(DisplayName = "Pistol"),
	SG		UMETA(DisplayName = "Shotgun"),
	SMG		UMETA(DisplayName = "Submachine Gun"),
	SR		UMETA(DisplayName = "Sniper Rifle")
};

