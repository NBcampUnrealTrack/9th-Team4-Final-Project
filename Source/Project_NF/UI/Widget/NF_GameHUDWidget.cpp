#include "UI/Widget/NF_GameHUDWidget.h"

#include "Engine/LocalPlayer.h"
#include "MVVMSubsystem.h"
#include "MVVMViewModelBase.h"
#include "View/MVVMView.h"
#include "GamePlay/BuildSystem/Core/NF_BuildTypes.h"
#include "UI/SubSystem/NF_ViewModelSubsystem.h"
#include "UI/ViewModel/Base/NF_ViewModelBase.h"
#include "UI/ViewModel/GamePlay/NF_GameFlowViewModel.h"
#include "UI/ViewModel/Slots/NF_SkillSlotViewModel.h"
#include "UI/ViewModel/Status/NF_PlayerStatusViewModel.h"

namespace
{
	const FName HUDViewModelSlotName(TEXT("ViewModel"));
}

void UNF_GameHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	InitializeHUDViewModels();
}

void UNF_GameHUDWidget::NativeDestruct()
{
	ReleaseHUDViewModels();
	Super::NativeDestruct();
}

void UNF_GameHUDWidget::InitializeHUDViewModels()
{
	if (bViewModelsInitialized) return;

	ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	if (!IsValid(LocalPlayer)) return;

	UNF_ViewModelSubsystem* Subsystem = LocalPlayer->GetSubsystem<UNF_ViewModelSubsystem>();
	if (!IsValid(Subsystem)) return;
	
	ViewModelSubsystem = Subsystem;
	Subsystem->RefreshViewModelContext();

	UNF_PlayerStatusViewModel* PlayerStatusVM = NewObject<UNF_PlayerStatusViewModel>(this);
	RegisterOwnedViewModel(PlayerStatusVM);

	UNF_SkillSlotViewModel* ActiveSkillOneVM = NewObject<UNF_SkillSlotViewModel>(this);
	ActiveSkillOneVM->ConfigureSlot(ENF_BuildItemType::ActiveSkill, 0);
	RegisterOwnedViewModel(ActiveSkillOneVM);

	UNF_SkillSlotViewModel* ActiveSkillTwoVM = NewObject<UNF_SkillSlotViewModel>(this);
	ActiveSkillTwoVM->ConfigureSlot(ENF_BuildItemType::ActiveSkill, 1);
	RegisterOwnedViewModel(ActiveSkillTwoVM);

	UNF_SkillSlotViewModel* PassiveSkillVM = NewObject<UNF_SkillSlotViewModel>(this);
	PassiveSkillVM->ConfigureSlot(ENF_BuildItemType::PassiveSkill, 0);
	RegisterOwnedViewModel(PassiveSkillVM);

	UNF_GameFlowViewModel* GameFlowVM = NewObject<UNF_GameFlowViewModel>(this);
	RegisterOwnedViewModel(GameFlowVM);

	// 각 자식 WBP에 이름이 ViewModel인 Manual MVVM 슬롯 필요
	bool bAllAssigned = true;
	bAllAssigned &= AssignViewModel(PlayerStatusWidget.Get(), PlayerStatusVM);
	bAllAssigned &= AssignViewModel(ActiveSkillSlotOne.Get(), ActiveSkillOneVM);
	bAllAssigned &= AssignViewModel(ActiveSkillSlotTwo.Get(), ActiveSkillTwoVM);
	bAllAssigned &= AssignViewModel(PassiveSkillSlot.Get(), PassiveSkillVM);
	bAllAssigned &= AssignViewModel(MatchStatusWidget.Get(), GameFlowVM);

	if (!bAllAssigned)
	{
		UE_LOG(LogTemp, Error, TEXT("[NF HUD] ViewModel 연결 실패. 자식 WBP의 슬롯 이름, 타입, Manual 설정을 확인하세요."));
		ReleaseHUDViewModels();
		return;
	}

	bViewModelsInitialized = true;
}

void UNF_GameHUDWidget::RegisterOwnedViewModel(UNF_ViewModelBase* ViewModel)
{
	UNF_ViewModelSubsystem* Subsystem = ViewModelSubsystem.Get();
	if (!IsValid(Subsystem) || !IsValid(ViewModel)) return;

	OwnedViewModels.Add(ViewModel);
	Subsystem->RegisterViewModel(ViewModel);
}

bool UNF_GameHUDWidget::AssignViewModel(UUserWidget* TargetWidget, UMVVMViewModelBase* ViewModel)
{
	if (!IsValid(TargetWidget))
	{
		UE_LOG(LogTemp, Error, TEXT("[NF HUD] 대상 자식 위젯이 없습니다."));
		return false;
	}

	UMVVMView* View = UMVVMSubsystem::GetViewFromUserWidget(TargetWidget);
	if (!IsValid(View))
	{
		// 해제 시에는 자식의 MVVM View가 이미 종료되었을 수 있다.
		if (IsValid(ViewModel))
		{
			UE_LOG(LogTemp, Error, TEXT("[NF HUD] %s에 MVVM View가 없습니다."), *TargetWidget->GetName());
		}
		return false;
	}

	TScriptInterface<INotifyFieldValueChanged> ViewModelInterface;
	if (IsValid(ViewModel))
	{
		ViewModelInterface.SetObject(ViewModel);
		ViewModelInterface.SetInterface(ViewModel);
	}

	const bool bAssigned = View->SetViewModel(HUDViewModelSlotName, ViewModelInterface);
	if (!bAssigned && IsValid(ViewModel))
	{
		UE_LOG(
			LogTemp, 
			Error, 
			TEXT("[NF HUD] %s의 ViewModel 슬롯에 %s 지정 실패."),
			*TargetWidget->GetName(), 
			*ViewModel->GetClass()->GetName());
	}
	return bAssigned;
}

void UNF_GameHUDWidget::ClearChildViewModels()
{
	AssignViewModel(PlayerStatusWidget.Get(), nullptr);
	AssignViewModel(ActiveSkillSlotOne.Get(), nullptr);
	AssignViewModel(ActiveSkillSlotTwo.Get(), nullptr);
	AssignViewModel(PassiveSkillSlot.Get(), nullptr);
	AssignViewModel(MatchStatusWidget.Get(), nullptr);
}

void UNF_GameHUDWidget::ReleaseHUDViewModels()
{
	if (!OwnedViewModels.IsEmpty())
	{
		ClearChildViewModels();
	}

	UNF_ViewModelSubsystem* Subsystem = ViewModelSubsystem.Get();
	for (UNF_ViewModelBase* ViewModel : OwnedViewModels)
	{
		if (!IsValid(ViewModel)) continue;
		
		if (IsValid(Subsystem))
		{
			Subsystem->UnregisterViewModel(ViewModel);
		}
		else
		{
			ViewModel->DeinitializeViewModel();
		}
	}

	OwnedViewModels.Reset();
	ViewModelSubsystem.Reset();
	bViewModelsInitialized = false;
}
