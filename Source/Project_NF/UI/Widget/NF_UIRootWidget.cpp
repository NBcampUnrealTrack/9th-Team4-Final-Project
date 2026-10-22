// Fill out your copyright notice in the Description page of Project Settings.
#include "UI/Widget/NF_UIRootWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"

void UNF_UIRootWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RootOverlay"));
	
	RootOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = RootOverlay;
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	// Overlay 자식 순서로 레이어 순서를 고정한다. 개별 ZOrder는 사용하지 않는다.
	HUDLayer = CreateLayer(RootOverlay, TEXT("HUDLayer"), ESlateVisibility::HitTestInvisible);
	ScreenLayer = CreateLayer(RootOverlay, TEXT("ScreenLayer"), ESlateVisibility::SelfHitTestInvisible);
	ModalLayer = CreateLayer(RootOverlay, TEXT("ModalLayer"), ESlateVisibility::SelfHitTestInvisible);
	OverlayLayer = CreateLayer(RootOverlay, TEXT("OverlayLayer"), ESlateVisibility::HitTestInvisible);
}

UOverlay* UNF_UIRootWidget::CreateLayer(UOverlay* Parent, const FName LayerName, const ESlateVisibility InVisibility)
{
	UOverlay* Layer = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), LayerName);
	Layer->SetVisibility(InVisibility);
	UOverlaySlot* LayerSlot = Parent->AddChildToOverlay(Layer);
	LayerSlot->SetHorizontalAlignment(HAlign_Fill);
	LayerSlot->SetVerticalAlignment(VAlign_Fill);
	return Layer;
}
