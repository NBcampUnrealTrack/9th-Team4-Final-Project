
#include "NF_PlayerController.h"

#include "EnhancedInputComponent.h"
#include "Data/HelperFunction/NF_GASFunctionLibrary.h"
#include "Input/MotionMatchingInputLibrary.h"
#include "Player/Data/InputActionDataAsset.h"



void ANF_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UMotionMatchingInputLibrary::BindMotionMatchingInput(this, MotionMatchingInputDataAsset);
	
	auto* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (IsValid(EnhancedInputComponent) == false) return;
	
	
}


void ANF_PlayerController::BindInputActions(UEnhancedInputComponent* EnhancedInputComponent)
{
	if (IsValid(EnhancedInputComponent) == true)
	{
		EnhancedInputComponent->BindAction(InputActionDataAsset->SkillButtonA, ETriggerEvent::Triggered, this, &ThisClass::TrySkillActionA);
	}
}


void ANF_PlayerController::TrySkillActionA()
{
	/*FGameplayTag() 이 부분을 UI가 브로드캐스트로 전달한 Tag값을 바인딩하는 함수에서
	  tag를 전달받아서 그걸 여기에 작성하면 된다. 그럼 나는 미리 UI의 브로드캐스트를 바인딩 해놔야겠지?*/
	UNF_GASFunctionLibrary::NF_TryAbility(this, FGameplayTag());
}