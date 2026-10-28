#include "UI/Widget/NF_MatchResultWidget.h"

#include "Components/Button.h"
#include "Engine/LocalPlayer.h"
#include "MVVMSubsystem.h"
#include "View/MVVMView.h"
#include "UI/SubSystem/NF_ViewModelSubsystem.h"
#include "UI/ViewModel/GamePlay/NF_MatchResultViewModel.h"

void UNF_MatchResultWidget::EnsureViewModel()
{
	if (!IsValid(ResultViewModel))
	{
		ResultViewModel = NewObject<UNF_MatchResultViewModel>(this);
	}
}

void UNF_MatchResultWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureViewModel();

	if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
	{
		ViewModelSubsystem = LocalPlayer->GetSubsystem<UNF_ViewModelSubsystem>();
		if (UNF_ViewModelSubsystem* Subsystem = ViewModelSubsystem.Get())
		{
			Subsystem->RegisterViewModel(ResultViewModel.Get());
		}
	}

	AssignViewModel(true);
	if (IsValid(ReturnButton))
	{
		ReturnButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleReturnClicked);
	}
}

void UNF_MatchResultWidget::NativeDestruct()
{
	if (IsValid(ReturnButton))
	{
		ReturnButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleReturnClicked);
	}
	AssignViewModel(false);

	if (IsValid(ResultViewModel))
	{
		if (UNF_ViewModelSubsystem* Subsystem = ViewModelSubsystem.Get())
		{
			Subsystem->UnregisterViewModel(ResultViewModel.Get());
		}
		else
		{
			ResultViewModel->DeinitializeViewModel();
		}
	}
	ViewModelSubsystem.Reset();
	Super::NativeDestruct();
}

void UNF_MatchResultWidget::AssignViewModel(const bool bAssign)
{
	UMVVMView* View = UMVVMSubsystem::GetViewFromUserWidget(this);
	if (!IsValid(View))
	{
		if (bAssign)
		{
			UE_LOG(LogTemp, Error, TEXT("[NF MatchResult] %s에 MVVM View가 없습니다."), *GetName());
		}
		return;
	}

	TScriptInterface<INotifyFieldValueChanged> ViewModelInterface;
	if (bAssign && IsValid(ResultViewModel))
	{
		ViewModelInterface.SetObject(ResultViewModel.Get());
		ViewModelInterface.SetInterface(ResultViewModel.Get());
	}

	const bool bAssigned = View->SetViewModel(FName(TEXT("ViewModel")), ViewModelInterface);
	if (bAssign && !bAssigned)
	{
		UE_LOG(LogTemp, Error, TEXT("[NF MatchResult] %s의 Manual ViewModel 슬롯 연결에 실패했습니다."), *GetName());
	}
}

void UNF_MatchResultWidget::ApplyResult(const FNF_MatchResult& Result, const int32 LocalTeamId)
{
	EnsureViewModel();
	ResultViewModel->ApplyResult(Result, LocalTeamId);
}

void UNF_MatchResultWidget::SetReturnPending(const bool bPending)
{
	EnsureViewModel();
	ResultViewModel->SetReturnPending(bPending);
}

void UNF_MatchResultWidget::HandleReturnClicked()
{
	if (!IsValid(ResultViewModel) || !ResultViewModel->GetCanReturn())
	{
		return;
	}
	if (!OnReturnRequested.IsBound())
	{
		UE_LOG(LogTemp, Warning, TEXT("[NF MatchResult] 복귀 요청 처리자가 연결되지 않았습니다."));
		return;
	}

	// 콜백에서 화면이 제거되거나 복귀가 거절될 수 있으므로 먼저 중복 클릭을 막는다.
	ResultViewModel->SetReturnPending(true);
	OnReturnRequested.Broadcast();
}
