#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "NFGA_SkillBase.generated.h"

// @Owner: 스킬 담당 | 2026-10-12
// 목적: 장착 유형과 기능 분류를 분리하여 공통 스킬 GA의 설정 정보를 제공한다.
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

/**
 * @Owner: 스킬 담당 | 2026-10-12
 * 목적: 개별 스킬이 상속하는 공통 Gameplay Ability 베이스.
 *
 * 분류, 발동 방식, 시간은 설정 정보이며 입력 연결, 자동 발동, 토글, 대기를 구현하지 않는다.
 * Blueprint 자식은 GAS의 ActivateAbility / ActivateAbilityFromEvent에서 실행을 구성한다.
 * 사용 확정 시 CommitAbility의 성공 여부를 확인하고, 완료 시 EndAbility를 호출한다.
 * 취소/종료 시 정리는 GAS의 OnEndAbility 이벤트에서 처리한다.
 * 지속 효과와 중첩은 GE가 관리하며, GA 실행 시간과 독립적이다.
 */
UCLASS(Abstract, Blueprintable)
class PROJECT_NF_API UNFGA_SkillBase : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UNFGA_SkillBase();

	UFUNCTION(BlueprintPure, Category = "NF|Skill")
	ENF_SkillType GetSkillType() const { return SkillType; }

	UFUNCTION(BlueprintPure, Category = "NF|Skill")
	ENF_SkillCategory GetSkillCategory() const { return SkillCategory; }

	UFUNCTION(BlueprintPure, Category = "NF|Skill")
	ENF_SkillActivationMethod GetSkillActivationMethod() const { return SkillActivationMethod; }

	UFUNCTION(BlueprintPure, Category = "NF|Skill")
	int32 GetBuildCost() const { return BuildCost; }

	UFUNCTION(BlueprintPure, Category = "NF|Skill")
	ENF_SkillExecutionMode GetSkillExecutionMode() const { return SkillExecutionMode; }

	UFUNCTION(BlueprintPure, Category = "NF|Skill")
	float GetCastTime() const { return CastTime; }

	UFUNCTION(BlueprintPure, Category = "NF|Skill")
	float GetExecutionDuration() const { return ExecutionDuration; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Classification")
	ENF_SkillType SkillType = ENF_SkillType::Active;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Classification")
	ENF_SkillCategory SkillCategory = ENF_SkillCategory::Utility;

	// 실제 발동 연결은 입력/장착 시스템 또는 GAS Ability Triggers에서 설정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Activation")
	ENF_SkillActivationMethod SkillActivationMethod = ENF_SkillActivationMethod::Input;

	// GA 자체의 실행 방식. 버프/디버프의 지속시간은 GE에서 별도로 설정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Execution")
	ENF_SkillExecutionMode SkillExecutionMode = ENF_SkillExecutionMode::Instant;

	// 실행 전 준비 시간(초). 자식 GA에서 AbilityTask 등을 사용해 대기를 구성한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Execution", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CastTime = 0.0f;

	// Timed 실행에서 사용할 시간(초). Continuous 실행은 자식의 종료 조건을 사용한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Execution", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "SkillExecutionMode == ENF_SkillExecutionMode::Timed"))
	float ExecutionDuration = 0.0f;

	// 장착 예산용 값. 사용 자원과 쿨타임은 GAS의 Cost/Cooldown GE를 사용한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Build", meta = (ClampMin = "0", UIMin = "0"))
	int32 BuildCost = 0;
};
