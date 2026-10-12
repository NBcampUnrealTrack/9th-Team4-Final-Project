#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MotionMatchingInputDataAsset.generated.h"

class UInputAction;
class UInputMappingContext;

// [변경] v0.0.2: 클래스 이름에 데이터 에셋임을 표시 (구 UMotionMatchingInputConfig, CoreRedirect로 기존 에셋 연결)
// [추가] A-3: 모션매칭 입력 에셋 묶음 (IMC + IA 10개 + 행동별 자동 연결 on/off).
// 특정 행동을 GA 등 다른 방식으로 처리하려면 해당 행동의 자동 연결만 끈다
UCLASS(BlueprintType)
class PROJECT_NF_API UMotionMatchingInputDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MotionMatching|Input")
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MotionMatching|Input")
	int32 MappingContextPriority = 0;

#pragma region Move
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MotionMatching|Input|Move")
	bool bIsMoveBindingEnabled = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MotionMatching|Input|Move", meta = (EditCondition = "bIsMoveBindingEnabled"))
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MotionMatching|Input|Move", meta = (EditCondition = "bIsMoveBindingEnabled"))
	TObjectPtr<UInputAction> MoveWorldSpaceAction;
#pragma endregion

#pragma region Look
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MotionMatching|Input|Look")
	bool bIsLookBindingEnabled = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MotionMatching|Input|Look", meta = (EditCondition = "bIsLookBindingEnabled"))
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MotionMatching|Input|Look", meta = (EditCondition = "bIsLookBindingEnabled"))
	TObjectPtr<UInputAction> LookGamepadAction;
#pragma endregion

#pragma region Sprint
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MotionMatching|Input|Sprint")
	bool bIsSprintBindingEnabled = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MotionMatching|Input|Sprint", meta = (EditCondition = "bIsSprintBindingEnabled"))
	TObjectPtr<UInputAction> SprintAction;
#pragma endregion

#pragma region Walk
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MotionMatching|Input|Walk")
	bool bIsWalkBindingEnabled = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MotionMatching|Input|Walk", meta = (EditCondition = "bIsWalkBindingEnabled"))
	TObjectPtr<UInputAction> WalkAction;
#pragma endregion

#pragma region Strafe
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MotionMatching|Input|Strafe")
	bool bIsStrafeBindingEnabled = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MotionMatching|Input|Strafe", meta = (EditCondition = "bIsStrafeBindingEnabled"))
	TObjectPtr<UInputAction> StrafeAction;
#pragma endregion

#pragma region Aim
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MotionMatching|Input|Aim")
	bool bIsAimBindingEnabled = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MotionMatching|Input|Aim", meta = (EditCondition = "bIsAimBindingEnabled"))
	TObjectPtr<UInputAction> AimAction;
#pragma endregion

#pragma region Crouch
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MotionMatching|Input|Crouch")
	bool bIsCrouchBindingEnabled = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MotionMatching|Input|Crouch", meta = (EditCondition = "bIsCrouchBindingEnabled"))
	TObjectPtr<UInputAction> CrouchAction;
#pragma endregion

#pragma region Jump
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MotionMatching|Input|Jump")
	bool bIsJumpBindingEnabled = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MotionMatching|Input|Jump", meta = (EditCondition = "bIsJumpBindingEnabled"))
	TObjectPtr<UInputAction> JumpAction;
#pragma endregion
};
