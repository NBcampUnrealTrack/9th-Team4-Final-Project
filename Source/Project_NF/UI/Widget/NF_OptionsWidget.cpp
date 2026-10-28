#include "UI/Widget/NF_OptionsWidget.h"
#include "UI/ViewModel/Settings/NF_OptionsViewModel.h"
#include "UI/SubSystem/NF_UIManagerSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "INotifyFieldValueChanged.h"
#include "MVVMSubsystem.h"
#include "View/MVVMView.h"
#include "InputCoreTypes.h"

void UNF_OptionsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);
}
void UNF_OptionsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!OptionsViewModel) OptionsViewModel = NewObject<UNF_OptionsViewModel>(this);
	OptionsViewModel->Initialize(GetWorld());
	UMVVMView* View = UMVVMSubsystem::GetViewFromUserWidget(this);
	TScriptInterface<INotifyFieldValueChanged> Source;
	Source.SetObject(OptionsViewModel);
	Source.SetInterface(Cast<INotifyFieldValueChanged>(OptionsViewModel));
	if (!View || !View->SetViewModel(ViewModelName, Source))
	{
		UE_LOG(LogTemp, Error, TEXT("Options WBP %s requires a Manual NF_OptionsViewModel source named %s."), *GetName(), *ViewModelName.ToString());
	}
	OnOptionsViewModelReady();
}
void UNF_OptionsWidget::NativeDestruct()
{
	if (OptionsViewModel) OptionsViewModel->Shutdown();
	if (UMVVMView* View = UMVVMSubsystem::GetViewFromUserWidget(this))
	{
		View->SetViewModel(ViewModelName, TScriptInterface<INotifyFieldValueChanged>());
	}
	Super::NativeDestruct();
}
void UNF_OptionsWidget::Close()
{
	if (auto* Player = GetOwningLocalPlayer())
	{
		if (auto* UI = Player->GetSubsystem<UNF_UIManagerSubsystem>())
		{
			if (UI->RemoveWidget(this)) return;
		}
	}
	RemoveFromParent();
}
FReply UNF_OptionsWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	if (Event.GetKey() == EKeys::Escape || Event.GetKey() == EKeys::F10)
	{
		Close(); return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(Geometry, Event);
}
