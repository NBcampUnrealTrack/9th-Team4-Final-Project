#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ㅜㄹ_PlayerInputDataAsset.generated.h"

class UInputAction;
class UInputMappingContext;

UCLASS()
class PROJECT_NF_API Uㅜㄹ_PlayerInputDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerController|Input")
	TObjectPtr<UInputMappingContext> NF_PlayerIMC;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerController|Input")
	TObjectPtr<UInputAction> SkillButtonA;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerController|Input")
	TObjectPtr<UInputAction> SkillButtonB;
};
