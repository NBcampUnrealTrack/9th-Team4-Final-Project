#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NF_CrosshairWidget.generated.h"

UCLASS()
class PROJECT_NF_API UNF_CrosshairWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Crosshair")
	FLinearColor CrosshairColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Crosshair")
	FLinearColor OutlineColor = FLinearColor::Black;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Crosshair", meta=(ClampMin="0.0"))
	float CenterGap = 4.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Crosshair", meta=(ClampMin="1.0"))
	float ArmLength = 7.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Crosshair", meta=(ClampMin="1.0"))
	float LineThickness = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Crosshair", meta=(ClampMin="0.0"))
	float OutlineThickness = 1.f;
};
