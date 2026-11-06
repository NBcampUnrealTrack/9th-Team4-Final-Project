#include "UI/Widget/NF_TeamPanelWidget.h"

#include "Components/VerticalBox.h"
#include "INotifyFieldValueChanged.h"
#include "MVVMSubsystem.h"
#include "View/MVVMView.h"
#include "UI/Widget/NF_TeamMemberWidget.h"
#include "UI/ViewModel/Status/NF_TeamMemberViewModel.h"

namespace
{
    const FName MemberViewModelSlotName(TEXT("ViewModel"));
}

void UNF_TeamPanelWidget::SetMembers(const TArray<UNF_TeamMemberViewModel*>& InMembers)
{
    if (InMembers.IsEmpty())
    {
        ClearMemberWidgets();
        return;
    }
    if (!VB_Members) return;

    TSet<UNF_TeamMemberViewModel*> CurrentMembers;
    for (UNF_TeamMemberViewModel* MemberVM : InMembers)
    {
        if (IsValid(MemberVM))
        {
            CurrentMembers.Add(MemberVM);
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
    for (UNF_TeamMemberViewModel* MemberVM : InMembers)
    {
        if (!IsValid(MemberVM))continue;
        
        if (MemberWidgets.Contains(MemberVM))continue;
        
        CreateMemberWidget(MemberVM);
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
    ClearMemberWidgets();
    Super::NativeDestruct();
}
