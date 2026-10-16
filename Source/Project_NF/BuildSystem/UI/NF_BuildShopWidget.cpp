#include "BuildSystem/UI/NF_BuildShopWidget.h"

#include "Blueprint/WidgetTree.h"
#include "BuildSystem/Data/NF_BuildItemDefinitionDataAsset.h"
#include "BuildSystem/Data/NF_BuildRules.h"
#include "BuildSystem/Player/NF_BuildComponent.h"
#include "BuildSystem/Player/NF_BuildInputComponent.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/InputKeySelector.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Input/Events.h"

void UNF_BuildItemButton::Configure(FName InItemId, bool bInRemove)
{
	ItemId = InItemId;
	bRemove = bInRemove;
	OnClicked.AddUniqueDynamic(this, &UNF_BuildItemButton::HandleClicked);
}

void UNF_BuildItemButton::HandleClicked()
{
	OnItemClicked.Broadcast(ItemId, bRemove);
}

void UNF_BuildShopWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);
	if (WidgetTree->RootWidget)
	{
		return;
	}
	UBorder* Root = WidgetTree->ConstructWidget<UBorder>();
	Root->SetPadding(FMargin(24.f));
	Root->SetBrushColor(FLinearColor(0.03f, 0.04f, 0.06f, 0.96f));
	WidgetTree->RootWidget = Root;
	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
	Root->SetContent(Content);
	auto AddText = [this, Content](const TCHAR* Text)
	{
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
		Label->SetText(FText::FromString(Text));
		Content->AddChildToVerticalBox(Label);
		return Label;
	};
	AddText(TEXT("빌드 상점 — 스킬 선택 후 슬롯 1 / 2에 배치하세요"));
	StatusText = AddText(TEXT("빌드 규칙을 읽는 중입니다."));
	ResultText = AddText(TEXT("적용 전 선택은 임시 빌드입니다. 목록 오른쪽의 항목을 누르면 제거됩니다."));

	UHorizontalBox* SlotRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	Content->AddChildToVerticalBox(SlotRow);
	auto AddButton = [this](UPanelWidget* Parent, const TCHAR* Text)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>();
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
		Label->SetText(FText::FromString(Text));
		Button->SetContent(Label);
		Parent->AddChild(Button);
		return Button;
	};
	SlotOneButton = AddButton(SlotRow, TEXT("슬롯 1에 배치"));
	SlotOneButton->OnClicked.AddDynamic(this, &UNF_BuildShopWidget::HandleSlotOne);
	SlotOneKeySelector = WidgetTree->ConstructWidget<UInputKeySelector>();
	SlotRow->AddChild(SlotOneKeySelector);
	SlotOneKeySelector->SetAllowModifierKeys(false);
	SlotOneKeySelector->SetAllowGamepadKeys(false);
	SlotOneKeySelector->OnKeySelected.AddDynamic(this, &UNF_BuildShopWidget::HandleSlotOneKey);
	SlotTwoButton = AddButton(SlotRow, TEXT("슬롯 2에 배치"));
	SlotTwoButton->OnClicked.AddDynamic(this, &UNF_BuildShopWidget::HandleSlotTwo);
	SlotTwoKeySelector = WidgetTree->ConstructWidget<UInputKeySelector>();
	SlotRow->AddChild(SlotTwoKeySelector);
	SlotTwoKeySelector->SetAllowModifierKeys(false);
	SlotTwoKeySelector->SetAllowGamepadKeys(false);
	SlotTwoKeySelector->OnKeySelected.AddDynamic(this, &UNF_BuildShopWidget::HandleSlotTwoKey);

	UHorizontalBox* Lists = WidgetTree->ConstructWidget<UHorizontalBox>();
	Content->AddChildToVerticalBox(Lists)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	CatalogList = WidgetTree->ConstructWidget<UScrollBox>();
	DraftList = WidgetTree->ConstructWidget<UScrollBox>();
	Lists->AddChildToHorizontalBox(CatalogList)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Lists->AddChildToHorizontalBox(DraftList)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UHorizontalBox* Footer = WidgetTree->ConstructWidget<UHorizontalBox>();
	Content->AddChildToVerticalBox(Footer);
	ApplyButton = AddButton(Footer, TEXT("빌드 적용"));
	ApplyButton->OnClicked.AddDynamic(this, &UNF_BuildShopWidget::HandleApply);
	AddButton(Footer, TEXT("닫기 (Esc)"))->OnClicked.AddDynamic(this, &UNF_BuildShopWidget::HandleClose);
}

void UNF_BuildShopWidget::NativeConstruct()
{
	Super::NativeConstruct();
	APawn* Pawn = GetOwningPlayerPawn();
	Build = Pawn ? Pawn->FindComponentByClass<UNF_BuildComponent>() : nullptr;
	BuildInput = GetOwningPlayer() ? GetOwningPlayer()->FindComponentByClass<UNF_BuildInputComponent>() : nullptr;
	if (Build)
	{
		Build->OnDraftChanged.AddUniqueDynamic(this, &UNF_BuildShopWidget::Refresh);
		Build->OnBuildChanged.AddUniqueDynamic(this, &UNF_BuildShopWidget::Refresh);
		Build->OnBuildRequestCompleted.AddUniqueDynamic(this, &UNF_BuildShopWidget::HandleRequestCompleted);
	}
	if (BuildInput)
	{
		BuildInput->OnSlotKeysChanged.AddUniqueDynamic(this, &UNF_BuildShopWidget::Refresh);
	}
	Refresh();
}

void UNF_BuildShopWidget::NativeDestruct()
{
	if (Build)
	{
		Build->OnDraftChanged.RemoveDynamic(this, &UNF_BuildShopWidget::Refresh);
		Build->OnBuildChanged.RemoveDynamic(this, &UNF_BuildShopWidget::Refresh);
		Build->OnBuildRequestCompleted.RemoveDynamic(this, &UNF_BuildShopWidget::HandleRequestCompleted);
	}
	if (BuildInput)
	{
		BuildInput->OnSlotKeysChanged.RemoveDynamic(this, &UNF_BuildShopWidget::Refresh);
	}
	Super::NativeDestruct();
}

void UNF_BuildShopWidget::AddItemButton(UScrollBox* Container, FName ItemId, const FText& Label, bool bRemove)
{
	UNF_BuildItemButton* Button = WidgetTree->ConstructWidget<UNF_BuildItemButton>();
	Button->Configure(ItemId, bRemove);
	Button->OnItemClicked.AddDynamic(this, &UNF_BuildShopWidget::HandleItemClicked);
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
	Text->SetText(Label);
	Button->SetContent(Text);
	Button->SetIsEnabled(!Build->IsAwaitingBuildResult());
	Container->AddChild(Button);
}

void UNF_BuildShopWidget::Refresh()
{
	if (!Build || !StatusText || !CatalogList || !DraftList)
	{
		return;
	}
	const FNF_BuildValidation Validation = Build->PreviewBuild(Build->GetDraftBuild());
	const UNF_BuildItemDefinitionDataAsset* Pending = Build->FindItemDefinition(Build->GetPendingActiveSkill());
	StatusText->SetText(FText::Format(FText::FromString(TEXT("코스트 {0} / {1} — {2}\n선택 중: {3}")),
	                                  FText::AsNumber(Validation.TotalCost), FText::AsNumber(Validation.CostLimit),
	                                  Validation.Message,
	                                  Pending
		                                  ? Pending->DisplayName.IsEmpty()
			                                    ? FText::FromName(Pending->ItemId)
			                                    : Pending->DisplayName
		                                  : FText::FromString(TEXT("없음"))));
	const bool bEditing = !Build->IsAwaitingBuildResult();
	ApplyButton->SetIsEnabled(bEditing && !Pending && Validation.Result == ENF_BuildResult::Success);
	SlotOneButton->SetIsEnabled(bEditing && Pending);
	SlotTwoButton->SetIsEnabled(bEditing && Pending && Build->GetRules() && Build->GetRules()->ActiveSkillSlots >= 2);
	SlotOneKeySelector->SetIsEnabled(bEditing && BuildInput);
	SlotTwoKeySelector->SetIsEnabled(bEditing && BuildInput);
	if (BuildInput)
	{
		TGuardValue<bool> Guard(bRefreshingKeys, true);
		SlotOneKeySelector->SetSelectedKey(FInputChord(BuildInput->GetSlotKey(0)));
		SlotTwoKeySelector->SetSelectedKey(FInputChord(BuildInput->GetSlotKey(1)));
	}
	CatalogList->ClearChildren();
	DraftList->ClearChildren();
	if (Build->GetRules())
	{
		for (int32 TypeIndex = 0; TypeIndex <= static_cast<int32>(ENF_BuildItemType::PassiveSkill); ++TypeIndex)
		{
			const ENF_BuildItemType Type = static_cast<ENF_BuildItemType>(TypeIndex);
			UTextBlock* Heading = WidgetTree->ConstructWidget<UTextBlock>();
			Heading->SetText(StaticEnum<ENF_BuildItemType>()->GetDisplayNameTextByValue(TypeIndex));
			CatalogList->AddChild(Heading);
			for (const UNF_BuildItemDefinitionDataAsset* Item : Build->GetCatalogItems(Type, ENF_BuildArmorPart::None))
			{
				if (IsValid(Item))
				{
					const FText ItemName = Item->DisplayName.IsEmpty() ? FText::FromName(Item->ItemId) : Item->DisplayName;
					const FText Label = Item->ItemType == ENF_BuildItemType::Armor
						? FText::Format(FText::FromString(TEXT("[{0}] {1}")),
							StaticEnum<ENF_BuildArmorPart>()->GetDisplayNameTextByValue(static_cast<int64>(Item->ArmorPart)), ItemName)
						: ItemName;
					AddItemButton(CatalogList, Item->ItemId,
						FText::Format(FText::FromString(TEXT("{0} — 코스트 {1}")), Label, FText::AsNumber(Item->Cost)), false);
				}
			}
		}
	}
	for (const FNF_BuildSelection& Selection : Build->GetDraftBuild())
	{
		const UNF_BuildItemDefinitionDataAsset* Item = Build->FindItemDefinition(Selection.ItemId);
		AddItemButton(DraftList, Selection.ItemId,
		              FText::Format(FText::FromString(TEXT("{0} 슬롯 {1}: {2} (제거)")),
		                            Item && Item->ItemType == ENF_BuildItemType::Armor
			                            ? StaticEnum<ENF_BuildArmorPart>()->GetDisplayNameTextByValue(
				                            static_cast<int64>(Item->ArmorPart))
			                            : Item
			                            ? StaticEnum<ENF_BuildItemType>()->GetDisplayNameTextByValue(
				                            static_cast<int64>(Item->ItemType))
			                            : FText(),
		                            FText::AsNumber(Selection.SlotIndex + 1),
		                            Item && !Item->DisplayName.IsEmpty()
			                            ? Item->DisplayName
			                            : FText::FromName(Selection.ItemId)), true);
	}
}

void UNF_BuildShopWidget::HandleItemClicked(FName ItemId, bool bRemove)
{
	if (Build)
	{
		if (bRemove) Build->RemoveDraftItem(ItemId);
		else Build->SelectDraftItem(ItemId);
	}
}

void UNF_BuildShopWidget::HandleSlotOne() { if (Build) Build->AssignSelectedActiveSkill(0); }
void UNF_BuildShopWidget::HandleSlotTwo() { if (Build) Build->AssignSelectedActiveSkill(1); }
void UNF_BuildShopWidget::HandleApply() { if (Build) Build->ApplyDraftBuild(); }
void UNF_BuildShopWidget::HandleClose() { if (Build) Build->CloseBuildSession(); }
void UNF_BuildShopWidget::HandleSlotOneKey(FInputChord Chord) { ChangeKey(0, Chord); }
void UNF_BuildShopWidget::HandleSlotTwoKey(FInputChord Chord) { ChangeKey(1, Chord); }

void UNF_BuildShopWidget::ChangeKey(int32 SlotIndex, const FInputChord& Chord)
{
	if (bRefreshingKeys || !BuildInput || !Build || Build->IsAwaitingBuildResult()) return;
	const bool bChanged = !Chord.bCtrl && !Chord.bAlt && !Chord.bShift && !Chord.bCmd &&
		BuildInput->SetSlotKey(SlotIndex, Chord.Key);
	ResultText->SetText(FText::FromString(bChanged
		                                      ? TEXT("슬롯 키를 변경했습니다. 키 설정은 이 로컬 플레이어에 적용됩니다.")
		                                      : TEXT("사용할 수 없는 키입니다. 슬롯 중복, 이동·상호작용 입력 충돌, Esc, 조합키, 축 입력을 확인하세요.")));
	Refresh();
}

void UNF_BuildShopWidget::HandleRequestCompleted(const FNF_BuildValidation& Validation)
{
	if (ResultText) ResultText->SetText(Validation.Message);
	Refresh();
}

FReply UNF_BuildShopWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	if ((SlotOneKeySelector && SlotOneKeySelector->GetIsSelectingKey()) ||
		(SlotTwoKeySelector && SlotTwoKeySelector->GetIsSelectingKey()))
	{
		return Super::NativeOnPreviewKeyDown(Geometry, KeyEvent);
	}
	if (KeyEvent.GetKey() == EKeys::Escape)
	{
		HandleClose();
		return FReply::Handled();
	}
	if (BuildInput && Build && !KeyEvent.IsRepeat() && !Build->GetPendingActiveSkill().IsNone())
	{
		for (int32 SlotIndex = 0; SlotIndex < 2; ++SlotIndex)
		{
			if (KeyEvent.GetKey() == BuildInput->GetSlotKey(SlotIndex))
			{
				Build->AssignSelectedActiveSkill(SlotIndex);
				return FReply::Handled();
			}
		}
	}
	return Super::NativeOnPreviewKeyDown(Geometry, KeyEvent);
}
