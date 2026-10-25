#pragma once

#include "CoreMinimal.h"
#include "Data/Struct/NF_MatchResult.h"
#include "GameFramework/PlayerController.h"
#include "GamePlay/BuildSystem/Core/NF_BuildTypes.h"
#include "Player/Data/NF_PlayerActionType.h"
#include "Player/Data/NF_PlayerInteractTypeEnum.h"
#include "Player/Data/NF_PlayerStateDelegate.h"
#include "UI/Minimap/NF_MinimapRevealRegistry.h"
#include "NF_PlayerController.generated.h"

class UNF_GameHUDWidget;
enum class ENF_PlayerActionType : uint8;
enum class ENF_PlayerInteractTypeEnum : uint8;
enum class ENF_BuildShopUIState : uint8;
struct FInputActionValue;
class UNF_PlayerInputDataAsset;
class UMotionMatchingInputDataAsset;


UCLASS()
class PROJECT_NF_API ANF_PlayerController : public APlayerController
{
	GENERATED_BODY()
	


protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRep_PlayerState() override;
	
	virtual void BeginPlay() override;
	
	virtual void SetupInputComponent() override;
	

#pragma region Input
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerController|MotionMatching")
	TObjectPtr<UMotionMatchingInputDataAsset> MotionMatchingInputDataAsset;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerController|Input")
	TObjectPtr<UNF_PlayerInputDataAsset> NF_PlayerInputDataAsset;
	
	ENF_PlayerActionType NF_PlayerActionType = ENF_PlayerActionType::None;
	
	bool bIsToggleZoom = false;
	bool bIsZoomIn = false;
	
	void InputConfig();
	
	void BindInputButton();
	
	void BindDelegate();
	
	void TryAction(ENF_PlayerActionType PlayerActionType);
	
	void IsBuildShopAction(ENF_PlayerActionType PlayerActionType);
	
	void IsCombatAction(ENF_PlayerActionType PlayerActionType);
	
	void ZoomSelector(ENF_PlayerActionType PlayerActionType);
#pragma endregion 

	

#pragma region BuildShop
protected:
	UPROPERTY(Replicated)
	ENF_BuildShopUIState CurrentBuildShopType = ENF_BuildShopUIState::None;
	
	UFUNCTION(BlueprintCallable, Category = "NF|UI")
	void ReleaseGameplayInputForUI();
	
	void OnInteractBuildShop(ENF_BuildShopUIState BuildShopType);
#pragma endregion
	
#pragma region UI
protected:
	UPROPERTY(EditDefaultsOnly, Category = "NF|UI")
	TSubclassOf<UNF_GameHUDWidget> GameHUDClass;
private:
	void CreateGameHUD();
	void RefreshViewModel();
	
#pragma endregion 
	


#pragma region NotDevelopedByMe
public:
	UFUNCTION(Client, Reliable)
	void ClientHandleMatchCompleted(const FNF_MatchResult& Result);

	UFUNCTION(BlueprintCallable, Category = "NF|Minimap|Reveal")
	UNF_MinimapRevealRegistry* GetMinimapRevealRegistry();
	
private:
	// Server keeps disclosure reasons; only the owning client receives the snapshot.
	UPROPERTY(Transient)
	TObjectPtr<UNF_MinimapRevealRegistry> MinimapRevealRegistry;
	FTimerHandle MinimapRevealTimer;
	bool bSentMinimapReveals = false;
	void UpdateMinimapReveals();
	UFUNCTION(Client, Reliable)
	void ClientReceiveMinimapReveals(const TArray<FNF_MinimapRevealMarker>& Markers);
#pragma endregion 
};
