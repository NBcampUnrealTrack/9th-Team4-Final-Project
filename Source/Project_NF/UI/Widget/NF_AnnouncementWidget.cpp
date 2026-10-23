#include "UI/Widget/NF_AnnouncementWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"

void UNF_AnnouncementWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildWidgetTree();
}

void UNF_AnnouncementWidget::SetMessage(const FText& Message)
{
	if (IsValid(MessageText))
	{
		MessageText->SetText(Message);
	}
}

void UNF_AnnouncementWidget::BuildWidgetTree()
{
	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("AnnouncementCanvas"));
	WidgetTree->RootWidget = Canvas;

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("AnnouncementBackground"));
	Background->SetBrushColor(FLinearColor(0.025f, 0.035f, 0.055f, 0.82f));
	Background->SetPadding(FMargin(28.0f, 16.0f));
	if (UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Background))
	{
		CanvasSlot->SetAnchors(FAnchors(0.5f, 0.18f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetPosition(FVector2D::ZeroVector);
		CanvasSlot->SetAutoSize(true);
	}

	MessageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AnnouncementText"));
	MessageText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	MessageText->SetJustification(ETextJustify::Center);
	MessageText->SetAutoWrapText(true);
	FSlateFontInfo Font = MessageText->GetFont();
	Font.Size = 32;
	MessageText->SetFont(Font);
	Background->SetContent(MessageText);
}
