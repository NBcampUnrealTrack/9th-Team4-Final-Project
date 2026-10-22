// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NF_UIRootWidget.generated.h"

class UOverlay;

/** 화면 디자인과 분리된 로컬 플레이어 UI 레이어 컨테이너. */
UCLASS(NotBlueprintable)
class PROJECT_NF_API UNF_UIRootWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UOverlay* GetHUDLayer() const { return HUDLayer.Get(); }
	UOverlay* GetScreenLayer() const { return ScreenLayer.Get(); }
	UOverlay* GetModalLayer() const { return ModalLayer.Get(); }
	UOverlay* GetOverlayLayer() const { return OverlayLayer.Get(); }

protected:
	virtual void NativeOnInitialized() override;

private:
	UOverlay* CreateLayer(UOverlay* Parent, FName LayerName, ESlateVisibility InVisibility);

	UPROPERTY(Transient)
	TObjectPtr<UOverlay> HUDLayer;

	UPROPERTY(Transient)
	TObjectPtr<UOverlay> ScreenLayer;

	UPROPERTY(Transient)
	TObjectPtr<UOverlay> ModalLayer;

	UPROPERTY(Transient)
	TObjectPtr<UOverlay> OverlayLayer;
};
