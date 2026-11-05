#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "GamePlay/BuildSystem/Core/NF_BuildTypes.h"
#include "NF_BuildUISubsystem.generated.h"

class APawn;
class APlayerController;
class UNF_BuildComponent;
class UNF_BuildShopScreen;
class UNF_BuildShopViewModel;
class UWorld;

UCLASS()
class PROJECT_NF_API UNF_BuildSubSystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	UFUNCTION(BlueprintCallable, Category = "NF|Build|UI")
	void ConfigureScreen(TSubclassOf<UNF_BuildShopScreen> InScreenClass);

	// @Owner: 황준현 | 2026-10-31. 빙의 확인 후 현재 로컬 Pawn의 구독과 화면을 갱신한다.
	void RefreshPawnBinding();

	virtual void PlayerControllerChanged(APlayerController* NewPlayerController) override;
	virtual void Deinitialize() override;

private:

	void BindController(APlayerController* Controller);
	void UnbindController();

	void HandlePawnChanged(APawn* NewPawn);
	void SetObservedBuild(UNF_BuildComponent* NewBuild);

	UFUNCTION()
	void HandleShopStateChanged(ENF_BuildShopUIState State);

	void RefreshScreen();
	void CloseScreen();
	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	FDelegateHandle WorldCleanupHandle;
	TWeakObjectPtr<UWorld> BoundWorld;

	UPROPERTY(Transient)
	TObjectPtr<UNF_BuildShopViewModel> ShopViewModel;

	TWeakObjectPtr<APlayerController> BoundController;
	FDelegateHandle PawnChangedHandle;

	TWeakObjectPtr<UNF_BuildComponent> ObservedBuild;

	UPROPERTY(Transient)
	TSubclassOf<UNF_BuildShopScreen> ScreenClass;

	UPROPERTY(Transient)
	TObjectPtr<UNF_BuildShopScreen> Screen;

};
