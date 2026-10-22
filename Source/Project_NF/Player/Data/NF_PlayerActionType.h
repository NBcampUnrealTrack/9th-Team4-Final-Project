#pragma once

#include "CoreMinimal.h"
#include "NF_PlayerActionType.generated.h"

UENUM(BlueprintType)
enum class ENF_PlayerActionType : uint8
{
	None UMETA(DisplayName = "None"),
	MeleeAttack UMETA(DisplayName = "MeleeAttack"),
	Fire UMETA(DisplayName = "Fire"),
	StopFire UMETA(DisplayName = "StopFire"),
	ZoomIn UMETA(DisplayName = "ZoomIn"),
	ZoomOut UMETA(DisplayName = "ZoomOut"),
	Reload UMETA(DisplayName = "Reload"),
	Interaction UMETA(DisplayName = "Interaction"),
	SkillA UMETA(DisplayName = "SkillA"),
	SkillB UMETA(DisplayName = "SkillB"),
	ToggleSettingButton UMETA(DisplayName = "ToggleSettingButton")
};
