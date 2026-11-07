#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "GamePlay/BuildSystem/Core/NF_BuildTypesInterface.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "NF_BuildUISubsystem.generated.h"

class UNF_BuildShopViewModel;

// WBP_BuildCatalogEntry에 저장된 버튼 타입. 클릭 처리는 WBP에서 UButton::OnClicked에 연결한다.
UCLASS()
class PROJECT_NF_API UNF_BuildItemButton : public UButton
{
	GENERATED_BODY()
};

// @Owner: 황준현 | 2026-10-29. Model을 탐색하지 않는 빌드 상점 View.
UCLASS()
class PROJECT_NF_API UNF_BuildShopScreen : public UUserWidget
{
	GENERATED_BODY()

public:
	bool SetShopViewModel(UNF_BuildShopViewModel* InViewModel);

	UFUNCTION(BlueprintPure, Category = "NF|Build|UI")
	UNF_BuildShopViewModel* GetShopViewModel() const { return ShopViewModel; }

protected:
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

	UPROPERTY(EditDefaultsOnly, Category = "NF|Build|UI")
	FName ViewModelSlotName = TEXT("BuildShopVM");

	UPROPERTY(Transient, BlueprintReadOnly, Category = "NF|Build|UI")
	TObjectPtr<UNF_BuildShopViewModel> ShopViewModel;
};

class APawn;
class APlayerController;
class UNF_BuildComponent;
class UNF_BuildShopScreen;
class UNF_BuildShopViewModel;
class UWorld;

UCLASS()
class PROJECT_NF_API UNF_BuildUISubSystem : public ULocalPlayerSubsystem
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
