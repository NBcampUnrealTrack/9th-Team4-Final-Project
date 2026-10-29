#include "GamePlay/BuildSystem/UI/NF_BuildShopScreen.h"

#include "GamePlay/BuildSystem/UI/NF_BuildShopViewModelBridge.h"
#include "UI/ViewModel/BuildShop/NF_BuildShopViewModel.h"
#include "INotifyFieldValueChanged.h"
#include "InputCoreTypes.h"
#include "MVVMSubsystem.h"
#include "View/MVVMView.h"

bool UNF_BuildShopScreen::SetShopViewModel(UNF_BuildShopViewModel* InViewModel)
{
	UMVVMView* View = UMVVMSubsystem::GetViewFromUserWidget(this);
	if (!View) return false;

	ShopViewModel = InViewModel;
	TScriptInterface<INotifyFieldValueChanged> Source;
	Source.SetObject(InViewModel);
	Source.SetInterface(Cast<INotifyFieldValueChanged>(InViewModel));
	if (!View->SetViewModel(ViewModelSlotName, Source))
	{
		ShopViewModel = nullptr;
		UE_LOG(LogTemp, Error, TEXT("BuildShop WBP %s requires a Manual ViewModel slot named %s."),
			*GetName(), *ViewModelSlotName.ToString());
		return false;
	}
	return true;
}

FReply UNF_BuildShopScreen::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	if (Event.GetKey() == EKeys::Escape)
	{
		if (INF_BuildShopViewModelBridge* Bridge = Cast<INF_BuildShopViewModelBridge>(ShopViewModel.Get()))
		{
			Bridge->RequestClose();
			return FReply::Handled();
		}
	}
	return Super::NativeOnPreviewKeyDown(Geometry, Event);
}
