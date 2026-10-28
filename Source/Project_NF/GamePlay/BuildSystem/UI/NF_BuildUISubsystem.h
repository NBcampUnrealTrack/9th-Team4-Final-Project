#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "GamePlay/BuildSystem/Core/NF_BuildTypes.h"
#include "NF_BuildUISubsystem.generated.h"

class APawn;
class APlayerController;
class UNF_BuildComponent;
class UNF_BuildShopScreen;

UCLASS()
class PROJECT_NF_API UNF_BuildSubSystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="NF|Build|UI")
	void ConfigureScreen(TSubclassOf<UNF_BuildShopScreen> InScreenClass);
	
	virtual void PlayerControllerChanged(APlayerController* NewPlayerController) override;
	virtual void Deinitialize() override;
	
private:
	
	
	void BindController(APlayerController* Controller);
	void UnbindController();
	
	void HandlePawnChanged(APawn* NewPawn);
	void UnbindBuild();
	
	UFUNCTION()
	void HandleShopStateChanged(ENF_BuildShopUIState State);
	
	void RefreshScreen();
	void CloseScreen();
	
	TWeakObjectPtr<APlayerController> BoundController;
	FDelegateHandle PawnChangedHandle;
	
	TWeakObjectPtr<UNF_BuildComponent > ObservedBuild;
	
	UPROPERTY(Transient)
	TSubclassOf<UNF_BuildShopScreen> ScreenClass;
	
	UPROPERTY(Transient)
	TObjectPtr<UNF_BuildShopScreen> Screen;
	
	
};
