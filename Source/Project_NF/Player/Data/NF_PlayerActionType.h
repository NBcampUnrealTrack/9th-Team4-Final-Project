#pragma once

#include "CoreMinimal.h"
#include "NF_PlayerActionType.generated.h"

UENUM(BlueprintType)
enum class ENF_PlayerActionType : uint8
{
	None UMETA(DisplayName = "None"),
	Fire UMETA(DisplayName = "Fire"),
	StopFire UMETA(DisplayName = "StopFire"),
	Reload UMETA(DisplayName = "Reload"),
	Interaction UMETA(DisplayName = "Interaction"),
	SkillA UMETA(DisplayName = "SkillA"),
	SkillB UMETA(DisplayName = "SkillB"),
};
