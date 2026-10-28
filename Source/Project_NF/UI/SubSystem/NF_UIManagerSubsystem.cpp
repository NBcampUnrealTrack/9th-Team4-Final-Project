#include "UI/SubSystem/NF_UIManagerSubsystem.h"

#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Templates/UnrealTemplate.h"
#include "UI/Widget/NF_UIRootWidget.h"
#include "UI/Widget/NF_MatchResultWidget.h"
#include "UI/Widget/NF_OptionsWidget.h"
#include "Core/NF_GameState.h"

void UNF_UIManagerSubsystem::Deinitialize()
{
	TGuardValue<bool> Guard(bChangingUI, true);
	ResetUI();
	Super::Deinitialize();
}

void UNF_UIManagerSubsystem::PlayerControllerChanged(APlayerController* NewPlayerController)
{
	TGuardValue<bool> Guard(bChangingUI, true);
	ResetUI();
	Super::PlayerControllerChanged(NewPlayerController);
}

bool UNF_UIManagerSubsystem::EnsureRoot()
{
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!IsValid(LocalPlayer)){return false;}
	
	UWorld* World = LocalPlayer->GetWorld();
	if (!IsValid(World) || World->GetNetMode() == NM_DedicatedServer){return false;}
	
	APlayerController* Controller = LocalPlayer->GetPlayerController(World);
	if (!IsValid(Controller) || !Controller->IsLocalController()){return false;}
	
	if (IsValid(RootWidget) && OwningController.Get() == Controller && TimerWorld.Get() == World){return true;}

	ResetUI();
	UNF_UIRootWidget* NewRoot = CreateWidget<UNF_UIRootWidget>(Controller, UNF_UIRootWidget::StaticClass());
	if (!IsValid(NewRoot) || !NewRoot->AddToPlayerScreen(0)){return false;}
	
	RootWidget = NewRoot;
	OwningController = Controller;
	TimerWorld = World;
	return true;
}

UUserWidget* UNF_UIManagerSubsystem::CreateManagedWidget(TSubclassOf<UUserWidget> WidgetClass)
{
	if (!WidgetClass || WidgetClass->HasAnyClassFlags(CLASS_Abstract) || !EnsureRoot()){return nullptr;}
	// 옵션창은 Blueprint에서 직접 PushScreen/PushModal로 열 수도 있다.
	if (WidgetClass->IsChildOf(UNF_OptionsWidget::StaticClass()) && !CanOpenGameplayScreen()){return nullptr;}
	
	return CreateWidget<UUserWidget>(OwningController.Get(), WidgetClass);
}

void UNF_UIManagerSubsystem::AddToLayer(UOverlay* Layer, UUserWidget* Widget)
{
	check(IsValid(Layer) && IsValid(Widget));
	UOverlaySlot* LayerSlot = Layer->AddChildToOverlay(Widget);
	LayerSlot->SetHorizontalAlignment(HAlign_Fill);
	LayerSlot->SetVerticalAlignment(VAlign_Fill);
}

UUserWidget* UNF_UIManagerSubsystem::SetHUD(TSubclassOf<UUserWidget> WidgetClass)
{
	if (bChangingUI || !WidgetClass){return nullptr;}
	
	TGuardValue<bool> Guard(bChangingUI, true);
	if (!EnsureRoot()){return nullptr;}
	
	if (IsValid(HUDWidget) && HUDWidget->GetClass() == WidgetClass.Get()){return HUDWidget.Get();}
	
	UUserWidget* NewHUD = CreateManagedWidget(WidgetClass);
	if (!IsValid(NewHUD)){return nullptr;}
	
	UUserWidget* PreviousHUD = HUDWidget.Get();
	HUDWidget = NewHUD;
	
	if (IsValid(PreviousHUD))
	{
		PreviousHUD->RemoveFromParent();
	}
	AddToLayer(RootWidget->GetHUDLayer(), NewHUD);
	return NewHUD;
}

UUserWidget* UNF_UIManagerSubsystem::PushScreen(TSubclassOf<UUserWidget> WidgetClass, const ENF_ScreenInputMode InputMode,
	const bool bAllowUserClose)
{
	if (bChangingUI){return nullptr;}
	
	TGuardValue<bool> Guard(bChangingUI, true);
	
	UUserWidget* Widget = CreateManagedWidget(WidgetClass);
	if (!IsValid(Widget)){return nullptr;}
	
	Widget->SetIsFocusable(true);
	Widget->SetVisibility(ESlateVisibility::Collapsed);
	FNF_ScreenEntry& Entry = ScreenStack.AddDefaulted_GetRef();
	Entry.Widget = Widget;
	Entry.InputMode = InputMode;
	Entry.bAllowUserClose = bAllowUserClose;
	AddToLayer(RootWidget->GetScreenLayer(), Widget);
	RefreshPresentation();
	return Widget;
}

UUserWidget* UNF_UIManagerSubsystem::PushModal(TSubclassOf<UUserWidget> WidgetClass)
{
	if (bChangingUI){return nullptr;}
	
	TGuardValue<bool> Guard(bChangingUI, true);
	UUserWidget* Widget = CreateManagedWidget(WidgetClass);
	if (!IsValid(Widget)){return nullptr;}
	
	Widget->SetIsFocusable(true);
	Widget->SetVisibility(ESlateVisibility::Collapsed);
	ModalStack.Add(Widget);
	AddToLayer(RootWidget->GetModalLayer(), Widget);
	RefreshPresentation();
	return Widget;
}

UUserWidget* UNF_UIManagerSubsystem::AddOverlay(TSubclassOf<UUserWidget> WidgetClass, const float Lifetime)
{
	if (bChangingUI || !FMath::IsFinite(Lifetime)){return nullptr;}
	
	TGuardValue<bool> Guard(bChangingUI, true);
	UUserWidget* Widget = CreateManagedWidget(WidgetClass);
	if (!IsValid(Widget)){return nullptr;}
	
	FNF_OverlayEntry& Entry = OverlayEntries.AddDefaulted_GetRef();
	Entry.Widget = Widget;
	if (Lifetime > 0.0f)
	{
		TWeakObjectPtr<UUserWidget> WeakWidget = Widget;
		FTimerDelegate ExpireDelegate = FTimerDelegate::CreateWeakLambda(this, [this, WeakWidget]()
		{
			RemoveWidget(WeakWidget.Get());
		});
		TimerWorld->GetTimerManager().SetTimer(Entry.LifetimeTimer, ExpireDelegate, Lifetime, false);
	}
	AddToLayer(RootWidget->GetOverlayLayer(), Widget);
	return Widget;
}

bool UNF_UIManagerSubsystem::RemoveWidget(UUserWidget* Widget)
{
	if (bChangingUI || !IsValid(Widget)){return false;}
	
	TGuardValue<bool> Guard(bChangingUI, true);
	return RemoveWidgetInternal(Widget);
}

bool UNF_UIManagerSubsystem::RemoveWidgetInternal(UUserWidget* Widget)
{
	if (HUDWidget.Get() == Widget)
	{
		HUDWidget = nullptr;
		Widget->RemoveFromParent();
		return true;
	}
	
	const int32 ScreenIndex = ScreenStack.IndexOfByPredicate(
		[Widget](const FNF_ScreenEntry& Entry) { return Entry.Widget.Get() == Widget; });
	
	if (ScreenIndex != INDEX_NONE)
	{
		ScreenStack.RemoveAt(ScreenIndex);
		Widget->RemoveFromParent();
		RefreshPresentation();
		return true;
	}
	
	const int32 ModalIndex = ModalStack.IndexOfByPredicate(
		[Widget](const TObjectPtr<UUserWidget>& Entry) { return Entry.Get() == Widget; });
	
	if (ModalIndex != INDEX_NONE)
	{
		ModalStack.RemoveAt(ModalIndex);
		Widget->RemoveFromParent();
		RefreshPresentation();
		return true;
	}
	
	const int32 OverlayIndex = OverlayEntries.IndexOfByPredicate(
		[Widget](const FNF_OverlayEntry& Entry) { return Entry.Widget.Get() == Widget; });
	
	if (OverlayIndex != INDEX_NONE)
	{
		ClearOverlayTimer(OverlayEntries[OverlayIndex]);
		OverlayEntries.RemoveAt(OverlayIndex);
		Widget->RemoveFromParent();
		return true;
	}
	
	return false;
}

UUserWidget* UNF_UIManagerSubsystem::GetTopInteractiveWidget() const
{
	if (!ModalStack.IsEmpty())
	{
		return ModalStack.Last().Get();
	}
	if (!ScreenStack.IsEmpty())
	{
		return ScreenStack.Last().Widget.Get();
	}
	return nullptr;
}

bool UNF_UIManagerSubsystem::CloseTop()
{
	if (bChangingUI){return false;}
	if (!ModalStack.IsEmpty())
	{
		return RemoveWidget(ModalStack.Last().Get());
	}
	if (ScreenStack.IsEmpty() || !ScreenStack.Last().bAllowUserClose){return false;}
	return RemoveWidget(ScreenStack.Last().Widget.Get());
}

bool UNF_UIManagerSubsystem::CanOpenGameplayScreen() const
{
	const UWorld* World = GetWorld();
	if (!IsValid(World)){return false;}
	const ANF_GameState* GameState = World->GetGameState<ANF_GameState>();
	if (IsValid(GameState) && GameState->GetMatchPhase() == ENF_MatchPhase::Finished){return false;}
	for (const FNF_ScreenEntry& Entry : ScreenStack)
	{
		if (IsValid(Entry.Widget) && Entry.Widget->IsA<UNF_MatchResultWidget>()){return false;}
	}
	return true;
}

UNF_MatchResultWidget* UNF_UIManagerSubsystem::ShowMatchResult(
	TSubclassOf<UNF_MatchResultWidget> WidgetClass, const FNF_MatchResult& Result, const int32 LocalTeamId)
{
	if (bChangingUI || !WidgetClass || WidgetClass->HasAnyClassFlags(CLASS_Abstract) ||
		Result.Outcome == ENF_MatchOutcome::None){return nullptr;}
	if (!EnsureRoot()){return nullptr;}
	for (const FNF_ScreenEntry& Entry : ScreenStack)
	{
		if (UNF_MatchResultWidget* Existing = Cast<UNF_MatchResultWidget>(Entry.Widget.Get()))
		{
			Existing->ApplyResult(Result, LocalTeamId);
			return Existing;
		}
	}
	ClearTransientUI();
	UNF_MatchResultWidget* Widget = Cast<UNF_MatchResultWidget>(
		PushScreen(WidgetClass, ENF_ScreenInputMode::UIOnly, false));
	if (IsValid(Widget))
	{
		Widget->ApplyResult(Result, LocalTeamId);
	}
	return Widget;
}

bool UNF_UIManagerSubsystem::IsGameplayInputBlocked() const
{
	return !ScreenStack.IsEmpty() || !ModalStack.IsEmpty();
}

void UNF_UIManagerSubsystem::RefreshPresentation()
{
	if (!IsValid(RootWidget))return;
	
	for (int32 Index = 0; Index < ScreenStack.Num(); ++Index)
	{
		ESlateVisibility Visibility = ESlateVisibility::Collapsed;
		if (Index == ScreenStack.Num() - 1)
		{
			Visibility = ESlateVisibility::SelfHitTestInvisible;
		}
		ScreenStack[Index].Widget->SetVisibility(Visibility);
	}
	for (int32 Index = 0; Index < ModalStack.Num(); ++Index)
	{
		ESlateVisibility Visibility = ESlateVisibility::Collapsed;
		if (Index == ModalStack.Num() - 1)
		{
			Visibility = ESlateVisibility::SelfHitTestInvisible;
		}
		ModalStack[Index]->SetVisibility(Visibility);
	}
	ESlateVisibility ScreenVisibility = ESlateVisibility::SelfHitTestInvisible;
	if (!ModalStack.IsEmpty())
	{
		ScreenVisibility = ESlateVisibility::HitTestInvisible;
	}
	RootWidget->GetScreenLayer()->SetVisibility(ScreenVisibility);
	ApplyInputMode();
}

void UNF_UIManagerSubsystem::ApplyInputMode()
{
	APlayerController* Controller = OwningController.Get();
	if (!IsValid(Controller) || !Controller->IsLocalController())return;
	
	UUserWidget* InputWidget = GetTopInteractiveWidget();
	if (!IsValid(InputWidget))
	{
		FInputModeGameOnly InputMode;
		Controller->bShowMouseCursor = false;
		Controller->SetInputMode(InputMode);
		return;
	}
	Controller->bShowMouseCursor = true;
	if (!ModalStack.IsEmpty() || ScreenStack.Last().InputMode == ENF_ScreenInputMode::UIOnly)
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(InputWidget->TakeWidget());
		Controller->SetInputMode(InputMode);
	}
	else
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(InputWidget->TakeWidget());
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Controller->SetInputMode(InputMode);
	}
}

void UNF_UIManagerSubsystem::ClearOverlayTimer(FNF_OverlayEntry& Entry)
{
	if (UWorld* World = TimerWorld.Get())
	{
		World->GetTimerManager().ClearTimer(Entry.LifetimeTimer);
	}
	Entry.LifetimeTimer.Invalidate();
}

void UNF_UIManagerSubsystem::ClearTransientUIInternal()
{
	for (FNF_OverlayEntry& Entry : OverlayEntries)
	{
		ClearOverlayTimer(Entry);
	}
	// 콜백이 발생하기 전에 관리 상태를 비운다. 실제 위젯은 Root가 소유한다.
	ScreenStack.Reset();
	ModalStack.Reset();
	OverlayEntries.Reset();
	if (IsValid(RootWidget))
	{
		RootWidget->GetModalLayer()->ClearChildren();
		RootWidget->GetScreenLayer()->ClearChildren();
		RootWidget->GetOverlayLayer()->ClearChildren();
	}
}

void UNF_UIManagerSubsystem::ClearTransientUI()
{
	if (bChangingUI)return;
	
	TGuardValue<bool> Guard(bChangingUI, true);
	ClearTransientUIInternal();
	RefreshPresentation();
}

void UNF_UIManagerSubsystem::ResetUI()
{
	const bool bHadInteractiveUI = IsGameplayInputBlocked();
	ClearTransientUIInternal();
	// 이전 Controller의 UI 입력만 복구한다. HUD/Overlay만 있었다면 입력을 건드리지 않는다.
	if (bHadInteractiveUI)
	{
		ApplyInputMode();
	}
	HUDWidget = nullptr;
	if (IsValid(RootWidget))
	{
		RootWidget->GetHUDLayer()->ClearChildren();
		RootWidget->RemoveFromParent();
		RootWidget = nullptr;
	}
	OwningController.Reset();
	TimerWorld.Reset();
}
