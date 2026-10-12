
#include "NF_PlayerController.h"
#include "Input/MotionMatchingInputLibrary.h" // [MotionMatchingInstaller]


// [MotionMatchingInstaller] 모션매칭 입력 연결
void ANF_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UMotionMatchingInputLibrary::BindMotionMatchingInput(this, MotionMatchingInputConfig); // [MotionMatchingInstaller]
}
