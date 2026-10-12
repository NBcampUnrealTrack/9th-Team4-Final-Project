#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MotionMatchingInputLibrary.generated.h"

class APlayerController;
class UEnhancedInputComponent;
class UMotionMatchingComponent;
class UMotionMatchingInputDataAsset;

// [추가] A-3: 컨트롤러 입력을 MotionMatchingComponent 행동 함수에 연결하는 함수 모음.
// 입력이 들어올 때마다 조종 중인 폰에서 컴포넌트를 찾으므로 리스폰·빙의가 바뀌어도 다시 연결할 필요 없음
UCLASS()
class PROJECT_NF_API UMotionMatchingInputLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 컨트롤러의 SetupInputComponent(또는 BP BeginPlay)에서 한 번 호출. 매핑 등록 + 켜진 행동만 연결
	UFUNCTION(BlueprintCallable, Category = "MotionMatching|Input")
	static bool BindMotionMatchingInput(APlayerController* PlayerController, const UMotionMatchingInputDataAsset* InputConfig);

	// 컨트롤러가 조종 중인 폰의 MotionMatchingComponent (없으면 nullptr)
	UFUNCTION(BlueprintPure, Category = "MotionMatching|Input")
	static UMotionMatchingComponent* GetControlledMotionMatchingComponent(const APlayerController* PlayerController);

private:
	static void AddMappingContext(APlayerController* PlayerController, const UMotionMatchingInputDataAsset* InputConfig);
	static void BindMoveInput(UEnhancedInputComponent* EnhancedInputComponent, APlayerController* PlayerController, const UMotionMatchingInputDataAsset* InputConfig);
	static void BindLookInput(UEnhancedInputComponent* EnhancedInputComponent, APlayerController* PlayerController, const UMotionMatchingInputDataAsset* InputConfig);
	static void BindStateInput(UEnhancedInputComponent* EnhancedInputComponent, APlayerController* PlayerController, const UMotionMatchingInputDataAsset* InputConfig);
	static void BindJumpInput(UEnhancedInputComponent* EnhancedInputComponent, APlayerController* PlayerController, const UMotionMatchingInputDataAsset* InputConfig);
};
