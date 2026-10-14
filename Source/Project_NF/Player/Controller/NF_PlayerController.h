#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/PlayerController.h"
#include "Player/Data/NF_PlayerInteractTypeEnum.h"
#include "Player/Data/NF_PlayerStateDelegate.h"
#include "NF_PlayerController.generated.h"

enum class ENF_PlayerInteractTypeEnum : uint8;
struct FInputActionValue;
class UNF_PlayerInputDataAsset;
class UMotionMatchingInputDataAsset;



UCLASS()
class PROJECT_NF_API ANF_PlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerController|MotionMatching")
	TObjectPtr<UMotionMatchingInputDataAsset> MotionMatchingInputDataAsset;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerController|Input")
	TObjectPtr<UNF_PlayerInputDataAsset> NF_PlayerInputDataAsset;
	
	UPROPERTY(ReplicatedUsing = OnRep_OnChangedPlayerStateType)
	ENF_PlayerInteractTypeEnum CurrentPlayerInteractType = ENF_PlayerInteractTypeEnum::None;
	
	FGameplayTag SelectedActiveSkill_Tag;
	FGameplayTag SkillButtonA_Tag;
	FGameplayTag SkillButtonB_Tag; 
	
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayerController|Interaction")
	float InteractionRange = 300.0f;
	
private:
	FOnChangedPlayerType OnChangedPlayerType;
	void OnChangedPlayerStateType(ENF_PlayerInteractTypeEnum NewPlayerType);
	
	/*UFUNCTION(Server, Reliable)
	void ServerRPC_OnChangedPlayerStateType(ENF_PlayerInteractTypeEnum NewPlayerType);*/
	
	UFUNCTION()
	void OnRep_OnChangedPlayerStateType();

	
	void BindActiveSkill();
	
	void TrySkillAction_A();
	void BindActiveSkillButton_A();
	
	void TrySkillAction_B();
	void BindActiveSkillButton_B();
	
	void TryInteraction();
	UFUNCTION(Server, Reliable)
	void ServerRPC_TryInteraction();
	
	
	FHitResult NF_Linetrace();
};
