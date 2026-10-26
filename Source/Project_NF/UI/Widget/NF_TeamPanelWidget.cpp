#include "UI/Widget/NF_TeamPanelWidget.h"

#include "Components/VerticalBox.h"
#include "GameFramework/PlayerState.h"
#include "UI/Widget/NF_TeamMemberWidget.h"

UNF_TeamMemberWidget* UNF_TeamPanelWidget::AddMember(APlayerState* PlayerState)
{
	if (!IsValid(PlayerState))
	{
		return nullptr;
	}
	
	if (UNF_TeamMemberWidget* Existing = FindMember(PlayerState))
	{
		return Existing;
	}
	
	if (!VB_Members || !MemberWidgetClass || !GetOwningPlayer())
	{
		return nullptr;
	}

	UNF_TeamMemberWidget* Entry = CreateWidget<UNF_TeamMemberWidget>(GetOwningPlayer(), MemberWidgetClass);
	if (!Entry)
	{
		return nullptr;
	}

	VB_Members->AddChildToVerticalBox(Entry);
	MemberWidgets.Add(PlayerState, Entry);
	Entry->SetMemberName(FText::FromString(PlayerState->GetPlayerName()));
	// 실제 상태를 전달받기 전의 초기 표시값.
	Entry->SetMemberLevel(1);
	Entry->SetHealth(0.0f, 0.0f);
	Entry->SetShield(0.0f, 0.0f);
	return Entry;
}

UNF_TeamMemberWidget* UNF_TeamPanelWidget::FindMember(APlayerState* PlayerState) const
{
	const TObjectPtr<UNF_TeamMemberWidget>* Found = MemberWidgets.Find(PlayerState);
	if (Found)
	{
		return Found->Get();
	}
	
	return nullptr;
}

void UNF_TeamPanelWidget::RemoveMember(APlayerState* PlayerState)
{
	if (UNF_TeamMemberWidget* Entry = FindMember(PlayerState))
	{
		Entry->RemoveFromParent();
	}
	
	MemberWidgets.Remove(PlayerState);
}

void UNF_TeamPanelWidget::ClearMembers()
{
	if (VB_Members)
	{
		VB_Members->ClearChildren();
	}
	
	MemberWidgets.Reset();
}

void UNF_TeamPanelWidget::NativeDestruct()
{
	ClearMembers();
	Super::NativeDestruct();
}
