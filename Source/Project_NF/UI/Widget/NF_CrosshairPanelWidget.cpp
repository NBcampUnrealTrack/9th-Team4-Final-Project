#include "UI/Widget/NF_CrosshairPanelWidget.h"

#include "Components/SizeBox.h"
#include "GameFramework/PlayerController.h"
#include "UI/ViewModel/GamePlay/NF_CrosshairViewModel.h"
#include "UI/Widget/NF_CrosshairWidget.h"

void UNF_CrosshairPanelWidget::SetCrosshairClass(TSubclassOf<UUserWidget> InClass)
{
	if (!IsValid(CrosshairSlot.Get()))return;

	if (IsValid(ActiveCrosshairWidget.Get()) &&ActiveCrosshairWidget->GetClass() == InClass.Get())return;

	if (IsValid(ActiveCrosshairWidget.Get()))
	{
		ActiveCrosshairWidget->RemoveFromParent();
	}
	ActiveCrosshairWidget = nullptr;

	if (InClass == nullptr)return;

	APlayerController* Controller = GetOwningPlayer();
	if (!IsValid(Controller) || !Controller->IsLocalController())return;

	UUserWidget* NewWidget = CreateWidget<UUserWidget>(Controller, InClass);
	if (!IsValid(NewWidget))return;
	
	CrosshairSlot->SetContent(NewWidget);
	NewWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	ActiveCrosshairWidget = NewWidget;
}

void UNF_CrosshairPanelWidget::SetCrosshairViewModel(UNF_CrosshairViewModel* NewViewModel)
{
	if (CrosshairViewModel.Get() == NewViewModel)return;

	if (IsValid(CrosshairViewModel.Get()))
	{
		CrosshairViewModel->OnShotFeedbackRequested.Remove(ShotFeedbackHandle);
	}
	ShotFeedbackHandle.Reset();
	CrosshairViewModel = NewViewModel;

	if (IsValid(NewViewModel))
	{
		ShotFeedbackHandle = NewViewModel->OnShotFeedbackRequested.AddUObject(this, &ThisClass::HandleShotFeedback);
	}
	else
	{
		SetCrosshairClass(TSubclassOf<UUserWidget>());
	}
}

void UNF_CrosshairPanelWidget::HandleShotFeedback()
{
	UNF_CrosshairWidget* Crosshair = Cast<UNF_CrosshairWidget>(ActiveCrosshairWidget.Get());
	if (IsValid(Crosshair))
	{
		Crosshair->NotifyShotFired();
	}
}

void UNF_CrosshairPanelWidget::NativeDestruct()
{
	SetCrosshairViewModel(nullptr);
	SetCrosshairClass(TSubclassOf<UUserWidget>());
	Super::NativeDestruct();
}
