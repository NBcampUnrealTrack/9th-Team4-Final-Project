#include "NF_ViewModelBase.h"

void UNF_ViewModelBase::InitializeViewModel(
	const FNF_ViewModelContext& NewContext)
{
	if (bIsInitialized)
	{
		UpdateViewModelContext(NewContext);
		return;
	}

	ViewModelContext = NewContext;
	bIsInitialized = true;

	OnInitializeViewModel();
}

void UNF_ViewModelBase::UpdateViewModelContext(
	const FNF_ViewModelContext& NewContext)
{
	if (!bIsInitialized)
	{
		InitializeViewModel(NewContext);
		return;
	}

	ViewModelContext = NewContext;
	OnViewModelContextChanged();
}

void UNF_ViewModelBase::DeinitializeViewModel()
{
	if (!bIsInitialized)
	{
		return;
	}

	OnDeinitializeViewModel();

	ViewModelContext.Reset();
	bIsInitialized = false;
}

