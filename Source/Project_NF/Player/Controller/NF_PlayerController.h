#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NF_PlayerController.generated.h"

class UMotionMatchingInputDataAsset; // [MotionMatchingInstaller]

UCLASS()
class PROJECT_NF_API ANF_PlayerController : public APlayerController
{
	GENERATED_BODY()
protected:
	// [MotionMatchingInstaller] 입력 에셋 묶음 (DA_MotionMatchingInputConfig_Default)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MotionMatching")
	TObjectPtr<UMotionMatchingInputDataAsset> MotionMatchingInputConfig;
	virtual void SetupInputComponent() override;

};
