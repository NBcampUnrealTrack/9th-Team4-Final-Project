#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "TimerManager.h"
#include "Data/Struct/NF_MatchResult.h"
#include "NF_UIManagerSubsystem.generated.h"

class APlayerController;
class UOverlay;
class UWorld;
class UNF_UIRootWidget;
class UNF_MatchResultWidget;


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

	// 일반 닫기만 제한한다. 시스템 제거는 허용한다.
	UPROPERTY(Transient)
	bool bAllowUserClose = true;
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
		TSubclassOf<UUserWidget> WidgetClass, ENF_ScreenInputMode InputMode = ENF_ScreenInputMode::GameAndUI,
		bool bAllowUserClose = true);

	// 기존 결과 화면은 갱신하고, 최초 표시 시 다른 임시 UI를 정리한다.
	UFUNCTION(BlueprintCallable, Category = "NF|UI")
	UNF_MatchResultWidget* ShowMatchResult(TSubclassOf<UNF_MatchResultWidget> WidgetClass,
		const FNF_MatchResult& Result, int32 LocalTeamId);

	// 종료된 매치 또는 표시 중인 결과 화면에서는 빌드샵/옵션 진입을 막는다.
	UFUNCTION(BlueprintPure, Category = "NF|UI")
	bool CanOpenGameplayScreen() const;

	// Modal은 UIOnly 입력 사용
	UFUNCTION(BlueprintCallable, Category = "NF|UI")
	UUserWidget* PushModal(TSubclassOf<UUserWidget> WidgetClass);

	// Lifetime > 0이면 자동 제거
	UFUNCTION(BlueprintCallable, Category = "NF|UI")
	UUserWidget* AddOverlay(TSubclassOf<UUserWidget> WidgetClass, float Lifetime = 0.0f);

	// 관리 위젯은 직접 RemoveFromParent 하지 않고 이 함수로 제거
	UFUNCTION(BlueprintCallable, Category = "NF|UI")
	bool RemoveWidget(UUserWidget* Widget);

	// Modal -> Screen 순서로 닫는다. 일반 닫기가 금지된 Screen은 유지한다.
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
