#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/PlayerController.h"
#include "Player/Data/NF_PlayerStateType.h"
#include "NF_PlayerController.generated.h"

enum class ENF_PlayerStateType : uint8;
struct FInputActionValue;
class UNF_PlayerInputDataAsset;
class UMotionMatchingInputDataAsset;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnChangedPlayerType, ENF_PlayerStateType);

UCLASS()
class PROJECT_NF_API ANF_PlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerController|MotionMatching")
	TObjectPtr<UMotionMatchingInputDataAsset> MotionMatchingInputDataAsset;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerController|Input")
	TObjectPtr<UNF_PlayerInputDataAsset> NF_PlayerInputDataAsset;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UAnimMontage> TestMontage;
	
	
	ENF_PlayerStateType CurrentPlayerStateType = ENF_PlayerStateType::None;
	
	FGameplayTag SelectedActiveSkill_Tag;
	FGameplayTag SkillButtonA_Tag;
	FGameplayTag SkillButtonB_Tag; 
	
	
private:
	FOnChangedPlayerType OnChangedPlayerType;
	void OnChangedPlayerStateType(ENF_PlayerStateType NewPlayerType);
	
	void BindActiveSkill();
	void BindActiveSkillButton_A();
	void BindActiveSkillButton_B();
	void TrySkillAction_A();
	void TrySkillAction_B();
	
};
