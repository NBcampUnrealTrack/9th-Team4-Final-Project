#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NF_SkillTypes.generated.h"

// 기존 enum 이름과 값은 BP 직렬화 호환을 위해 유지한다.
UENUM(BlueprintType)
enum class ENF_SkillType : uint8
{
	Active UMETA(DisplayName = "Active"),
	Passive UMETA(DisplayName = "Passive")
};

UENUM(BlueprintType)
enum class ENF_SkillCategory : uint8
{
	Attack UMETA(DisplayName = "Attack"),
	Utility UMETA(DisplayName = "Utility"),
	Movement UMETA(DisplayName = "Movement"),
	Search UMETA(DisplayName = "Search")
};

UENUM(BlueprintType)
enum class ENF_SkillActivationMethod : uint8
{
	Input UMETA(DisplayName = "Input"),
	OnGranted UMETA(DisplayName = "On Granted"),
	GameplayEvent UMETA(DisplayName = "Gameplay Event"),
	Toggle UMETA(DisplayName = "Toggle")
};

UENUM(BlueprintType)
enum class ENF_SkillExecutionMode : uint8
{
	Instant UMETA(DisplayName = "Instant"),
	Timed UMETA(DisplayName = "Timed"),
	Continuous UMETA(DisplayName = "Continuous")
};

USTRUCT(BlueprintType)
struct PROJECT_NF_API FNF_SkillStateRule
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag StateTag;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bBlockActivation = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bCancelRunning = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bCancelRunning"))
	bool bForceCancel = false;
};
