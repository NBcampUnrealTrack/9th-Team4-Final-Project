#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Player/Data/NF_PlayerActionType.h"
#include "Player/Data/NF_PlayerInteractTypeEnum.h"
#include "Player/Data/NF_PlayerStateDelegate.h"
#include "NF_PlayerController.generated.h"

enum class ENF_PlayerActionType : uint8;
enum class ENF_PlayerInteractTypeEnum : uint8;
struct FInputActionValue;
class UNF_PlayerInputDataAsset;
class UMotionMatchingInputDataAsset;



UCLASS()
class PROJECT_NF_API ANF_PlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:
	//virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void BeginPlay() override;
	
	virtual void SetupInputComponent() override;
	
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerController|MotionMatching")
	TObjectPtr<UMotionMatchingInputDataAsset> MotionMatchingInputDataAsset;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerController|Input")
	TObjectPtr<UNF_PlayerInputDataAsset> NF_PlayerInputDataAsset;
	
	//UPROPERTY(ReplicatedUsing = OnRep_OnChangedPlayerStateType)
	ENF_PlayerInteractTypeEnum CurrentPlayerInteractType = ENF_PlayerInteractTypeEnum::None;
	
	ENF_PlayerActionType NF_PlayerActionType = ENF_PlayerActionType::None;
	
	
private:
	void BindInputButton();
	
	void TryAction(ENF_PlayerActionType PlayerActionType);
	
	
#pragma region BuildShop
	//UFUNCTION()
	//void OnRep_OnChangedPlayerStateType();
	
	
	bool IsInBuildShop();
	
	
	FOnPlayerOpenShop OnPlayerOpenShop;
	
#pragma endregion
};
