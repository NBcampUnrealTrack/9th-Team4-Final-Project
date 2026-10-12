#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/PlayerController.h"
#include "NF_PlayerController.generated.h"

struct FInputActionValue;
class UInputActionDataAsset;
class UMotionMatchingInputDataAsset;

UCLASS()
class PROJECT_NF_API ANF_PlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	
protected:
	virtual void SetupInputComponent() override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerController|MotionMatching")
	TObjectPtr<UMotionMatchingInputDataAsset> MotionMatchingInputDataAsset;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerController|Input")
	TObjectPtr<UInputActionDataAsset> InputActionDataAsset;
	


	
private:
	void BindInputActions(UEnhancedInputComponent* EnhancedInputComponent);
	void TrySkillActionA();
};
