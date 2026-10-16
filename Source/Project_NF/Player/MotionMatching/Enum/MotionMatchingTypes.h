#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "MotionMatchingTypes.generated.h"

class UPrimitiveComponent;
class UAnimMontage;

// 걷기 / 달리기 / 질주
UENUM(BlueprintType)
enum class EMotionMatchingGait : uint8
{
	Walk UMETA(DisplayName = "Walk"),
	Run UMETA(DisplayName = "Run"),
	Sprint UMETA(DisplayName = "Sprint")
};


// 입력 상태 (다른 클라이언트에도 복제됨)
USTRUCT(BlueprintType)
struct FMotionMatchingInputState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	bool bIsWantsToSprint = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	bool bIsWantsToWalk = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	bool bIsWantsToStrafe = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	bool bIsWantsToAim = false;
};


// [추가] BP E_TraversalActionType 대체 (순서 동일: None, Vault, Hurdle, Mantle)
UENUM(BlueprintType)
enum class EMotionMatchingTraversalActionType : uint8
{
	None UMETA(DisplayName = "None"),
	Vault UMETA(DisplayName = "Vault"),
	Hurdle UMETA(DisplayName = "Hurdle"),
	Mantle UMETA(DisplayName = "Mantle")
};


// [추가] BP S_TraversalCheckInputs 대체: 앞쪽 캡슐 트레이스 설정값
USTRUCT(BlueprintType)
struct FMotionMatchingTraversalCheckInputs
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	FVector TraceForwardDirection = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	float TraceForwardDistance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	FVector TraceOriginOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	FVector TraceEndOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	float TraceRadius = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	float TraceHalfHeight = 0.0f;
};


// [추가] BP S_TraversalCheckResult 대체: 검사 결과 + 선택된 몽타주. 다른 클라이언트로 복제된다
USTRUCT(BlueprintType)
struct FMotionMatchingTraversalCheckResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	EMotionMatchingTraversalActionType ActionType = EMotionMatchingTraversalActionType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	bool bIsFrontLedgeFound = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	FVector FrontLedgeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	FVector FrontLedgeNormal = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	bool bIsBackLedgeFound = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	FVector BackLedgeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	FVector BackLedgeNormal = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	bool bIsBackFloorFound = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	FVector BackFloorLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	float ObstacleHeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	float ObstacleDepth = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	float BackLedgeHeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	TObjectPtr<UPrimitiveComponent> HitComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	TObjectPtr<UAnimMontage> ChosenMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	float StartTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	float PlayRate = 0.0f;
};


// [추가] BP S_TraversalChooserInputs 대체: CHT_TraversalAnims가 읽는 값 (Context 0, Read)
USTRUCT(BlueprintType)
struct FMotionMatchingTraversalChooserInputs
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	EMotionMatchingTraversalActionType ActionType = EMotionMatchingTraversalActionType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	bool bIsFrontLedgeFound = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	bool bIsBackLedgeFound = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	bool bIsBackFloorFound = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	float ObstacleHeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	float ObstacleDepth = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	float BackLedgeHeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	TEnumAsByte<EMovementMode> MovementMode = MOVE_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	EMotionMatchingGait Gait = EMotionMatchingGait::Walk;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	float Speed = 0.0f;
};


// [추가] BP S_TraversalChooserOutputs 대체: CHT_TraversalAnims가 써 주는 값 (Context 1, Write)
USTRUCT(BlueprintType)
struct FMotionMatchingTraversalChooserOutputs
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	EMotionMatchingTraversalActionType ActionType = EMotionMatchingTraversalActionType::None;
};
