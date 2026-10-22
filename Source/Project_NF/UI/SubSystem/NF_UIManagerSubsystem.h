#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "TimerManager.h"
#include "NF_UIManagerSubsystem.generated.h"

class APlayerController;
class UOverlay;
class UWorld;
class UNF_UIRootWidget;

UENUM(BlueprintType)
enum class ENF_ScreenInputMode : uint8
{
	GameAndUI,
	UIOnly
};

USTRUCT()
struct FNF_ScreenEntry
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> Widget;

	UPROPERTY(Transient)
	ENF_ScreenInputMode InputMode = ENF_ScreenInputMode::GameAndUI;
};

USTRUCT()
struct FNF_OverlayEntry
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> Widget;

	FTimerHandle LifetimeTimer;
};

/** 로컬 플레이어의 HUD, 화면/모달 스택, 비상호작용 Overlay와 입력 정책을 관리한다. */
UCLASS()
class PROJECT_NF_API UNF_UIManagerSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;
	virtual void PlayerControllerChanged(APlayerController* NewPlayerController) override;

	UFUNCTION(BlueprintCallable, Category = "NF|UI")
	UUserWidget* SetHUD(TSubclassOf<UUserWidget> WidgetClass);

	UFUNCTION(BlueprintCallable, Category = "NF|UI")
	UUserWidget* PushScreen(
		TSubclassOf<UUserWidget> WidgetClass,ENF_ScreenInputMode InputMode = ENF_ScreenInputMode::GameAndUI);

	// Modal은 UIOnly 입력 사용
	UFUNCTION(BlueprintCallable, Category = "NF|UI")
	UUserWidget* PushModal(TSubclassOf<UUserWidget> WidgetClass);

	// Lifetime > 0이면 자동 제거
	UFUNCTION(BlueprintCallable, Category = "NF|UI")
	UUserWidget* AddOverlay(TSubclassOf<UUserWidget> WidgetClass, float Lifetime = 0.0f);

	// 관리 위젯은 직접 RemoveFromParent 하지 않고 이 함수로 제거
	UFUNCTION(BlueprintCallable, Category = "NF|UI")
	bool RemoveWidget(UUserWidget* Widget);

	// Modal -> Screen 순서로 하나를 닫는다. HUD/Overlay는 제외
	UFUNCTION(BlueprintCallable, Category = "NF|UI")
	bool CloseTop();

	// HUD는 유지하고 Screen, Modal, Overlay를 제거
	UFUNCTION(BlueprintCallable, Category = "NF|UI")
	void ClearTransientUI();

	UFUNCTION(BlueprintPure, Category = "NF|UI")
	UUserWidget* GetHUD() const { return HUDWidget.Get(); }

	UFUNCTION(BlueprintPure, Category = "NF|UI")
	UUserWidget* GetTopInteractiveWidget() const;

	UFUNCTION(BlueprintPure, Category = "NF|UI")
	bool IsGameplayInputBlocked() const;

private:
	bool EnsureRoot();
	UUserWidget* CreateManagedWidget(TSubclassOf<UUserWidget> WidgetClass);
	void AddToLayer(UOverlay* Layer, UUserWidget* Widget);
	bool RemoveWidgetInternal(UUserWidget* Widget);
	void RefreshPresentation();
	void ApplyInputMode();
	void ClearOverlayTimer(FNF_OverlayEntry& Entry);
	void ClearTransientUIInternal();
	void ResetUI();

	UPROPERTY(Transient)
	TObjectPtr<UNF_UIRootWidget> RootWidget;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> HUDWidget;

	UPROPERTY(Transient)
	TArray<FNF_ScreenEntry> ScreenStack;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UUserWidget>> ModalStack;

	UPROPERTY(Transient)
	TArray<FNF_OverlayEntry> OverlayEntries;

	TWeakObjectPtr<APlayerController> OwningController;
	TWeakObjectPtr<UWorld> TimerWorld;

	bool bChangingUI = false;
};
