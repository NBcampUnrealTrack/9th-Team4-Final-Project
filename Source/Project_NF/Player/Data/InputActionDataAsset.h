#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InputActionDataAsset.generated.h"

class UInputAction;

UCLASS()
class PROJECT_NF_API UInputActionDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerController|Input")
	TObjectPtr<UInputAction> SkillButtonA;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerController|Input")
	TObjectPtr<UInputAction> SkillButtonB;
};
