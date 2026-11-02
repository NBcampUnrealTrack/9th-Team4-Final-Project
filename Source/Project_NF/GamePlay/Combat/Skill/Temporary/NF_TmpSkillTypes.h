#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NF_TmpSkillTypes.generated.h"

// @Owner: 이정준 (구현: Codex) | 2026-10-30 | 기존 스킬과 독립적인 구조 검토용 타입.
UENUM(BlueprintType)
enum class ENF_TmpSkillType : uint8 { Active, Passive };

UENUM(BlueprintType)
enum class ENF_TmpSkillCategory : uint8 { Attack, Utility, Movement, Search };

UENUM(BlueprintType)
enum class ENF_TmpSkillInput : uint8
{
	Press UMETA(DisplayName = "누를 때 사용"),
	Release UMETA(DisplayName = "놓을 때 사용"),
	Hold UMETA(DisplayName = "누르는 동안 유지")
};

USTRUCT(BlueprintType)
struct PROJECT_NF_API FNF_TmpSkillStateRule
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
