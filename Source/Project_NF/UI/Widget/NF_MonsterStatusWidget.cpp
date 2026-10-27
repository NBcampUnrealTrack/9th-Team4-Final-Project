#include "UI/Widget/NF_MonsterStatusWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "INotifyFieldValueChanged.h"
#include "Monster/NF_Monster.h"
#include "UI/ViewModel/Status/NF_MonsterStatusViewModel.h"

TSharedRef<SWidget> UNF_MonsterStatusWidget::RebuildWidget()
{
	EnsureNativeLayout();
	return Super::RebuildWidget();
}

void UNF_MonsterStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureNativeLayout();
	if (!IsValid(ViewModel) && ObservedMonster.IsValid()) BindObservedMonster();
	RefreshDisplay();
}

void UNF_MonsterStatusWidget::NativeDestruct()
{
	UnbindViewModel();
	Super::NativeDestruct();
}

void UNF_MonsterStatusWidget::EnsureNativeLayout()
{
	if (TXT_MonsterName && PB_Health) return;
	if (WidgetTree->RootWidget) return;

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MonsterStatusRoot"));
	WidgetTree->RootWidget = Root;
	TXT_MonsterName = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TXT_MonsterName"));
	PB_Health = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("PB_Health"));
	TXT_MonsterName->SetJustification(ETextJustify::Center);
	Root->AddChildToVerticalBox(TXT_MonsterName)->SetHorizontalAlignment(HAlign_Fill);
	PB_Health->SetFillColorAndOpacity(FLinearColor(0.8f, 0.05f, 0.05f, 1.0f));
	Root->AddChildToVerticalBox(PB_Health);
}

void UNF_MonsterStatusWidget::ObserveMonster(ANF_Monster* Monster)
{
	UnbindViewModel();
	ObservedMonster = Monster;
	BindObservedMonster();
}

void UNF_MonsterStatusWidget::RefreshMonsterName()
{
	if (IsValid(ViewModel) && ObservedMonster.IsValid())
	{
		ViewModel->SetUnitName(ObservedMonster->GetMonsterDisplayName());
	}
}

void UNF_MonsterStatusWidget::BindObservedMonster()
{
	ANF_Monster* Monster = ObservedMonster.Get();
	if (!IsValid(Monster)) return;

	ViewModel = NewObject<UNF_MonsterStatusViewModel>(this);
	ViewModel->SetObservedUnit(Monster->GetAbilitySystemComponent(), Monster->GetMonsterDisplayName());
	const auto& Descriptor = ViewModel->GetFieldNotificationDescriptor();
	HealthFieldId = Descriptor.GetField(ViewModel->GetClass(), TEXT("GetHealthPercent"));
	NameFieldId = Descriptor.GetField(ViewModel->GetClass(), TEXT("UnitName"));
	if (HealthFieldId.IsValid())
	{
		HealthChangedHandle = ViewModel->AddFieldValueChangedDelegate(HealthFieldId,
			INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &ThisClass::HandleViewModelChanged));
	}
	if (NameFieldId.IsValid())
	{
		NameChangedHandle = ViewModel->AddFieldValueChangedDelegate(NameFieldId,
			INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &ThisClass::HandleViewModelChanged));
	}
	RefreshDisplay();
}

void UNF_MonsterStatusWidget::UnbindViewModel()
{
	if (IsValid(ViewModel))
	{
		if (HealthChangedHandle.IsValid()) ViewModel->RemoveFieldValueChangedDelegate(HealthFieldId, HealthChangedHandle);
		if (NameChangedHandle.IsValid()) ViewModel->RemoveFieldValueChangedDelegate(NameFieldId, NameChangedHandle);
		ViewModel->ClearObservedUnit();
	}
	HealthChangedHandle.Reset();
	NameChangedHandle.Reset();
	HealthFieldId = UE::FieldNotification::FFieldId();
	NameFieldId = UE::FieldNotification::FFieldId();
	ViewModel = nullptr;
}

void UNF_MonsterStatusWidget::HandleViewModelChanged(UObject* Object, UE::FieldNotification::FFieldId FieldId)
{
	if (Object == ViewModel.Get()) RefreshDisplay();
}

void UNF_MonsterStatusWidget::RefreshDisplay()
{
	EnsureNativeLayout();
	if (!IsValid(ViewModel)) return;
	if (TXT_MonsterName) TXT_MonsterName->SetText(ViewModel->GetUnitName());
	if (PB_Health) PB_Health->SetPercent(ViewModel->GetHealthPercent());
}
