#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NF_PlayerController.generated.h"

struct FInputActionValue;
class UNF_PlayerInputDataAsset;
class UMotionMatchingInputDataAsset;

UCLASS()
class PROJECT_NF_API ANF_PlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	
protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerController|MotionMatching")
	TObjectPtr<UMotionMatchingInputDataAsset> MotionMatchingInputDataAsset;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerController|Input")
	TObjectPtr<UNF_PlayerInputDataAsset> NF_PlayerInputDataAsset;
	


	
private:
	void BindInputActions(UEnhancedInputComponent* EnhancedInputComponent);
	void TrySkillActionA();
	void TrySkillActionB();
	
};
