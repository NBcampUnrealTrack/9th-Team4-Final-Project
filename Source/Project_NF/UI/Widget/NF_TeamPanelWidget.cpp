#include "UI/Widget/NF_TeamPanelWidget.h"

#include "Components/VerticalBox.h"
#include "INotifyFieldValueChanged.h"
#include "MVVMSubsystem.h"
#include "View/MVVMView.h"
#include "UI/Widget/NF_TeamMemberWidget.h"
#include "UI/ViewModel/Status/NF_TeamMemberViewModel.h"
#include "UI/ViewModel/Status/NF_TeamPanelViewModel.h"

namespace
{
    const FName MemberViewModelSlotName(TEXT("ViewModel"));
    const FName MembersFieldName(TEXT("Members"));
}

void UNF_TeamPanelWidget::SetTeamPanelViewModel(UNF_TeamPanelViewModel* NewViewModel)
{
    if (TeamPanelViewModel.Get() == NewViewModel)
    {
        SynchronizeMembers();
        return;
    }

    UnbindViewModel();
    ClearMemberWidgets();
    TeamPanelViewModel = NewViewModel;

    if (!IsValid(TeamPanelViewModel.Get())) return;

    MembersFieldId = TeamPanelViewModel->GetFieldNotificationDescriptor().GetField(
        TeamPanelViewModel->GetClass(), MembersFieldName);
    if (!MembersFieldId.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[NF Team UI] Members FieldNotify를 찾지 못했습니다."));
        TeamPanelViewModel = nullptr;
        return;
    }

    MembersChangedHandle = TeamPanelViewModel->AddFieldValueChangedDelegate(
        MembersFieldId,
        INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(
            this, &ThisClass::HandleMembersChanged));

    // 구독 전에 생성된 팀원 목록도 즉시 표시한다.
    SynchronizeMembers();
}

void UNF_TeamPanelWidget::UnbindViewModel()
{
    if (IsValid(TeamPanelViewModel.Get()))
    {
        if (MembersChangedHandle.IsValid())
        {
            TeamPanelViewModel->RemoveFieldValueChangedDelegate(MembersFieldId, MembersChangedHandle);
        }
    }

    MembersChangedHandle.Reset();
    MembersFieldId = UE::FieldNotification::FFieldId();
    TeamPanelViewModel = nullptr;
}

void UNF_TeamPanelWidget::HandleMembersChanged(UObject* Object, UE::FieldNotification::FFieldId FieldId)
{
    if (Object != TeamPanelViewModel.Get())return;
    
    if (FieldId != MembersFieldId) return;

    SynchronizeMembers();
}

void UNF_TeamPanelWidget::SynchronizeMembers()
{
    if (!IsValid(TeamPanelViewModel.Get()))
    {
        ClearMemberWidgets();
        return;
    }
    if (!VB_Members) return;

    const TArray<TObjectPtr<UNF_TeamMemberViewModel>>& Members = TeamPanelViewModel->GetMembers();
    TSet<UNF_TeamMemberViewModel*> CurrentMembers;
    for (const TObjectPtr<UNF_TeamMemberViewModel>& MemberVM : Members)
    {
        if (IsValid(MemberVM.Get()))
        {
            CurrentMembers.Add(MemberVM.Get());
        }
    }

    // 빠진 팀원의 행은 ViewModel 연결을 해제한 뒤 제거한다.
    for (auto Iterator = MemberWidgets.CreateIterator(); Iterator; ++Iterator)
    {
        UNF_TeamMemberViewModel* MemberVM = Iterator.Key().Get();
        if (!CurrentMembers.Contains(MemberVM))
        {
            UNF_TeamMemberWidget* Widget = Iterator.Value().Get();
            if (IsValid(Widget))
            {
                AssignMemberViewModel(Widget, nullptr);
                Widget->RemoveFromParent();
            }
            Iterator.RemoveCurrent();
        }
    }

    // 기존 행은 유지하고 새 팀원만 마지막에 추가한다.
    for (const TObjectPtr<UNF_TeamMemberViewModel>& MemberVM : Members)
    {
        if (!IsValid(MemberVM.Get()))continue;
        
        if (MemberWidgets.Contains(MemberVM))continue;
        
        CreateMemberWidget(MemberVM.Get());
    }
}

void UNF_TeamPanelWidget::CreateMemberWidget(UNF_TeamMemberViewModel* MemberVM)
{
    if (!IsValid(MemberVM))return;
    
    if (!VB_Members)return;
    
    if (!MemberWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[NF Team UI] MemberWidgetClass를 지정하세요."));
        return;
    }
    if (!GetOwningPlayer())return;

    UNF_TeamMemberWidget* Widget = CreateWidget<UNF_TeamMemberWidget>(GetOwningPlayer(), MemberWidgetClass);
    if (!IsValid(Widget))return;

    VB_Members->AddChildToVerticalBox(Widget);
    if (!AssignMemberViewModel(Widget, MemberVM))
    {
        Widget->RemoveFromParent();
        return;
    }
    MemberWidgets.Add(MemberVM, Widget);
}

bool UNF_TeamPanelWidget::AssignMemberViewModel(UNF_TeamMemberWidget* Widget, UNF_TeamMemberViewModel* MemberVM)
{
    if (!IsValid(Widget)){return false;}

    UMVVMView* View = UMVVMSubsystem::GetViewFromUserWidget(Widget);
    if (!IsValid(View))
    {
        if (IsValid(MemberVM))
        {
            UE_LOG(LogTemp, Error, TEXT("[NF Team UI] %s에 MVVM View가 없습니다."), *Widget->GetName());
        }
        return false;
    }

    TScriptInterface<INotifyFieldValueChanged> ViewModelInterface;
    if (IsValid(MemberVM))
    {
        ViewModelInterface.SetObject(MemberVM);
        ViewModelInterface.SetInterface(MemberVM);
    }

    const bool bAssigned = View->SetViewModel(MemberViewModelSlotName, ViewModelInterface);
    if (!bAssigned && IsValid(MemberVM))
    {
        UE_LOG(LogTemp, Error, TEXT("[NF Team UI] %s의 Manual ViewModel 슬롯을 확인하세요."), *Widget->GetName());
    }
    return bAssigned;
}

void UNF_TeamPanelWidget::ClearMemberWidgets()
{
    for (const auto& Pair : MemberWidgets)
    {
        UNF_TeamMemberWidget* Widget = Pair.Value.Get();
        if (IsValid(Widget))
        {
            AssignMemberViewModel(Widget, nullptr);
            Widget->RemoveFromParent();
        }
    }
    MemberWidgets.Reset();
}

void UNF_TeamPanelWidget::NativeDestruct()
{
    UnbindViewModel();
    ClearMemberWidgets();
    Super::NativeDestruct();
}
