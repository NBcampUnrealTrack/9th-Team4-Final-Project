#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NF_PlayerControllerDataAsset.generated.h"

class UNF_GameHUDWidget;
class UInputAction;
class UInputMappingContext;

UCLASS()
class PROJECT_NF_API UNF_PlayerControllerDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, Category = "PlayerController|UI")
	TSubclassOf<UNF_GameHUDWidget> GameHUDClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerController|Input")
	TObjectPtr<UInputMappingContext> NF_PlayerIMC;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerController|Input")
	TObjectPtr<UInputAction> FireButton;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerController|Input")
	TObjectPtr<UInputAction> MeleeAttack;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerController|Input")
	TObjectPtr<UInputAction> ZoomButton;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerController|Input")
	TObjectPtr<UInputAction> ReloadButton;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerController|Input")
	TObjectPtr<UInputAction> InteractButton;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerController|Input")
	TObjectPtr<UInputAction> UseItemButton;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerController|Input")
	TObjectPtr<UInputAction> SkillButton_A;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerController|Input")
	TObjectPtr<UInputAction> SkillButton_B;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerController|Input")
	TObjectPtr<UInputAction> ZoomToggleSettingButton;
	

};
