#include "UI/SubSystem/NF_UIManagerSubsystem.h"

#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "UI/Widget/NF_AnnouncementWidget.h"

void UNF_UIManagerSubsystem::Deinitialize()
{
	PopAllWidgets();
	Super::Deinitialize();
}

void UNF_UIManagerSubsystem::PlayerControllerChanged(APlayerController* NewPlayerController)
{
	Super::PlayerControllerChanged(NewPlayerController);
	PopAllWidgets();
}

UUserWidget* UNF_UIManagerSubsystem::PushWidget(const TSubclassOf<UUserWidget> WidgetClass,
	const ENF_UIInputMode InputMode, const bool bShowMouseCursor, const int32 ZOrder)
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (!WidgetClass || !IsValid(PlayerController) || !PlayerController->IsLocalController())
	{
		return nullptr;
	}

	if (WidgetStack.IsEmpty())
	{
		bSavedMouseCursor = PlayerController->bShowMouseCursor;
		bHasSavedMouseCursor = true;
	}

	// Reuse a widget of the same class rather than stacking duplicates on repeated state notifications.
	for (int32 Index = WidgetStack.Num() - 1; Index >= 0; --Index)
	{
		if (UUserWidget* ExistingWidget = WidgetStack[Index].Widget.Get();
			IsValid(ExistingWidget) && ExistingWidget->IsA(WidgetClass.Get()))
		{
			FNFManagedWidgetEntry Entry = WidgetStack[Index];
			WidgetStack.RemoveAt(Index);
			Entry.InputMode = InputMode;
			Entry.bShowMouseCursor = bShowMouseCursor;
			ExistingWidget->RemoveFromParent();
			ExistingWidget->AddToPlayerScreen(ZOrder);
			WidgetStack.Add(MoveTemp(Entry));
			ApplyInputModeForTopWidget();
			return ExistingWidget;
		}
	}

	UUserWidget* Widget = CreateWidget<UUserWidget>(PlayerController, WidgetClass);
	if (!IsValid(Widget))
	{
		if (WidgetStack.IsEmpty())
		{
			bHasSavedMouseCursor = false;
		}
		return nullptr;
	}

	Widget->AddToPlayerScreen(ZOrder);
	FNFManagedWidgetEntry& Entry = WidgetStack.AddDefaulted_GetRef();
	Entry.Widget = Widget;
	Entry.InputMode = InputMode;
	Entry.bShowMouseCursor = bShowMouseCursor;
	ApplyInputModeForTopWidget();
	return Widget;
}

bool UNF_UIManagerSubsystem::RemoveWidget(UUserWidget* Widget)
{
	if (!IsValid(Widget))
	{
		return false;
	}

	const int32 Index = WidgetStack.IndexOfByPredicate(
		[Widget](const FNFManagedWidgetEntry& Entry)
		{
			return Entry.Widget.Get() == Widget;
		});
	if (Index == INDEX_NONE)
	{
		return false;
	}

	WidgetStack[Index].Widget->RemoveFromParent();
	WidgetStack.RemoveAt(Index);
	ApplyInputModeForTopWidget();
	return true;
}

bool UNF_UIManagerSubsystem::PopTopWidget()
{
	return RemoveWidget(GetTopWidget());
}

void UNF_UIManagerSubsystem::PopAllWidgets()
{
	for (FNFManagedWidgetEntry& Entry : WidgetStack)
	{
		if (IsValid(Entry.Widget))
		{
			Entry.Widget->RemoveFromParent();
		}
	}
	WidgetStack.Reset();
	ApplyInputModeForTopWidget();
}

UUserWidget* UNF_UIManagerSubsystem::GetTopWidget() const
{
	for (int32 Index = WidgetStack.Num() - 1; Index >= 0; --Index)
	{
		if (IsValid(WidgetStack[Index].Widget))
		{
			return WidgetStack[Index].Widget;
		}
	}
	return nullptr;
}

void UNF_UIManagerSubsystem::ShowAnnouncement(const FText& Message, const float Duration)
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController) || !PlayerController->IsLocalController())
	{
		return;
	}

	UNF_AnnouncementWidget* Widget = CreateWidget<UNF_AnnouncementWidget>(PlayerController);
	if (!IsValid(Widget))
	{
		return;
	}

	Widget->AddToPlayerScreen(1000);
	Widget->ShowAnnouncement(Message, Duration);
}

void UNF_UIManagerSubsystem::RefreshInputMode()
{
	ApplyInputModeForTopWidget();
}

APlayerController* UNF_UIManagerSubsystem::GetOwningPlayerController() const
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	return IsValid(LocalPlayer) ? LocalPlayer->GetPlayerController(LocalPlayer->GetWorld()) : nullptr;
}

void UNF_UIManagerSubsystem::ApplyInputModeForTopWidget()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController))
	{
		return;
	}

	int32 TopIndex = INDEX_NONE;
	for (int32 Index = WidgetStack.Num() - 1; Index >= 0; --Index)
	{
		if (IsValid(WidgetStack[Index].Widget))
		{
			TopIndex = Index;
			break;
		}
	}

	if (TopIndex == INDEX_NONE)
	{
		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = bHasSavedMouseCursor ? bSavedMouseCursor : false;
		bHasSavedMouseCursor = false;
		return;
	}

	const FNFManagedWidgetEntry& Entry = WidgetStack[TopIndex];
	PlayerController->bShowMouseCursor = Entry.bShowMouseCursor;
	switch (Entry.InputMode)
	{
	case ENF_UIInputMode::GameOnly:
	{
		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
		break;
	}
	case ENF_UIInputMode::UIOnly:
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(Entry.Widget->TakeWidget());
		PlayerController->SetInputMode(InputMode);
		break;
	}
	case ENF_UIInputMode::GameAndUI:
	default:
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(Entry.Widget->TakeWidget());
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
		break;
	}
	}
}
