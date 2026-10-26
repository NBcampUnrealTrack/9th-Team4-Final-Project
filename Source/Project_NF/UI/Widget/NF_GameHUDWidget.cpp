#include "UI/Widget/NF_GameHUDWidget.h"

#include "Engine/LocalPlayer.h"
#include "MVVMSubsystem.h"
#include "MVVMViewModelBase.h"
#include "View/MVVMView.h"
#include "GamePlay/BuildSystem/Core/NF_BuildTypes.h"
#include "UI/SubSystem/NF_ViewModelSubsystem.h"
#include "UI/Minimap/NF_MinimapWidget.h"
#include "UI/Widget/NF_PlayerStatusWidget.h"
#include "UI/Widget/NF_SkillSlotWidget.h"
#include "UI/Widget/NF_TeamPanelWidget.h"
#include "UI/Widget/NF_WeaponStatusWidget.h"
#include "UI/ViewModel/Base/NF_ViewModelBase.h"
#include "UI/ViewModel/GamePlay/NF_GameFlowViewModel.h"
#include "UI/ViewModel/GamePlay/NF_MinimapViewModel.h"
#include "UI/ViewModel/Slots/NF_SkillSlotViewModel.h"
#include "UI/ViewModel/Status/NF_PlayerStatusViewModel.h"
#include "UI/ViewModel/Status/NF_TeamPanelViewModel.h"
#include "UI/ViewModel/Status/NF_WeaponStatusViewModel.h"

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
	if (bViewModelsInitialized)return;

	ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	if (!IsValid(LocalPlayer))return;

	UNF_ViewModelSubsystem* Subsystem = LocalPlayer->GetSubsystem<UNF_ViewModelSubsystem>();
	if (!IsValid(Subsystem))return;
	
	
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

	UNF_MinimapViewModel* MinimapVM = NewObject<UNF_MinimapViewModel>(this);
	RegisterOwnedViewModel(MinimapVM);

	// 개별 TeamMemberViewModel은 TeamPanelVM이 생성하고 해제한다.
	TeamPanelVM = NewObject<UNF_TeamPanelViewModel>(this);
	RegisterOwnedViewModel(TeamPanelVM.Get());

	WeaponStatusVM = NewObject<UNF_WeaponStatusViewModel>(this);
	RegisterOwnedViewModel(WeaponStatusVM.Get());
	
	// 각 자식 WBP에 이름이 ViewModel인 Manual MVVM 슬롯 필요
	bool bAllAssigned = true;
	if (!AssignViewModel(PlayerStatusWidget.Get(), PlayerStatusVM)){bAllAssigned = false;}
	if (!AssignViewModel(ActiveSkillSlotOne.Get(), ActiveSkillOneVM)){bAllAssigned = false;}
	if (!AssignViewModel(ActiveSkillSlotTwo.Get(), ActiveSkillTwoVM)){bAllAssigned = false;}
	if (!AssignViewModel(PassiveSkillSlot.Get(), PassiveSkillVM)){bAllAssigned = false;}
	if (!AssignViewModel(MatchStatusWidget.Get(), GameFlowVM)){bAllAssigned = false;}
	if (!AssignViewModel(MinimapWidget.Get(), MinimapVM)){bAllAssigned = false;}
	if (!AssignViewModel(TeamPanelWidget.Get(), TeamPanelVM.Get())){bAllAssigned = false;}
	if (!AssignViewModel(WeaponStatusWidget.Get(), WeaponStatusVM.Get())){bAllAssigned = false;}
	
	if (!bAllAssigned)
	{
		UE_LOG(LogTemp, Error, TEXT("[NF HUD] ViewModel 연결 실패. 자식 WBP의 슬롯 이름, 타입, Manual 설정을 확인하세요."));
		ReleaseHUDViewModels();
		return;
	}

	TeamPanelWidget->SetTeamPanelViewModel(TeamPanelVM.Get());
	bViewModelsInitialized = true;
}

void UNF_GameHUDWidget::RegisterOwnedViewModel(UNF_ViewModelBase* ViewModel)
{
	UNF_ViewModelSubsystem* Subsystem = ViewModelSubsystem.Get();
	if (!IsValid(Subsystem))return;
	
	if (!IsValid(ViewModel))return;
	

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
	if (IsValid(TeamPanelWidget.Get()))
	{
		TeamPanelWidget->SetTeamPanelViewModel(nullptr);
	}

	AssignViewModel(PlayerStatusWidget.Get(), nullptr);
	AssignViewModel(ActiveSkillSlotOne.Get(), nullptr);
	AssignViewModel(ActiveSkillSlotTwo.Get(), nullptr);
	AssignViewModel(PassiveSkillSlot.Get(), nullptr);
	AssignViewModel(MatchStatusWidget.Get(), nullptr);
	AssignViewModel(MinimapWidget.Get(), nullptr);
	AssignViewModel(TeamPanelWidget.Get(), nullptr);
	AssignViewModel(WeaponStatusWidget.Get(), nullptr);
}

void UNF_GameHUDWidget::ReleaseHUDViewModels()
{
	if (!OwnedViewModels.IsEmpty())
	{
		ClearChildViewModels();
	}

	UNF_ViewModelSubsystem* Subsystem = ViewModelSubsystem.Get();
	for (const TObjectPtr<UNF_ViewModelBase>& ViewModel : OwnedViewModels)
	{
		if (!IsValid(ViewModel.Get()))continue;
		
		if (IsValid(Subsystem))
		{
			Subsystem->UnregisterViewModel(ViewModel.Get());
		}
		else
		{
			ViewModel->DeinitializeViewModel();
		}
	}

	OwnedViewModels.Reset();
	TeamPanelVM = nullptr;
	WeaponStatusVM = nullptr;
	ViewModelSubsystem.Reset();
	bViewModelsInitialized = false;
}

UNF_TeamPanelViewModel* UNF_GameHUDWidget::GetTeamPanelViewModel() const
{
	return TeamPanelVM.Get();
}

UNF_WeaponStatusViewModel* UNF_GameHUDWidget::GetWeaponStatusViewModel() const
{
	return WeaponStatusVM.Get();
}
