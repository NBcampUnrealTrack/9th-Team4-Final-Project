#include "UI/NF_CrosshairWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"
#include "GamePlay/Combat/Weapons/NF_WeaponComponent.h"
#include "Rendering/DrawElements.h"

void UNF_CrosshairWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		WidgetTree->RootWidget = WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("CrosshairRoot"));
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
	// Reevaluate possession/equipment even when the viewport uses invalidation caching.
	ForceVolatile(true);
}

int32 UNF_CrosshairWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 BaseLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect,
		OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	const APlayerController* Controller = GetOwningPlayer();
	const APawn* Pawn = GetOwningPlayerPawn();
	const UNF_WeaponComponent* Weapons = Pawn ? Pawn->FindComponentByClass<UNF_WeaponComponent>() : nullptr;
	if (!Controller || !Controller->IsLocalController() || !Weapons ||
		!IsValid(Weapons->GetEquippedWeapon()) || Controller->bShowMouseCursor ||
		(Controller->GetHUD() && !Controller->GetHUD()->bShowHUD))
	{
		return BaseLayer;
	}

	const FVector2D Center = AllottedGeometry.GetLocalSize() * 0.5f;
	const FVector2D Directions[] = { FVector2D(1, 0), FVector2D(-1, 0), FVector2D(0, 1), FVector2D(0, -1) };
	const float Gap = FMath::Max(0.f, CenterGap);
	const float Length = FMath::Max(1.f, ArmLength);
	const float Thickness = FMath::Max(1.f, LineThickness);
	const FLinearColor Tint = InWidgetStyle.GetColorAndOpacityTint();
	for (const FVector2D& Direction : Directions)
	{
		TArray<FVector2D> Points;
		Points.Add(Center + Direction * Gap);
		Points.Add(Center + Direction * (Gap + Length));
		if (OutlineThickness > 0.f)
		{
			FSlateDrawElement::MakeLines(OutDrawElements, BaseLayer + 1,
				AllottedGeometry.ToPaintGeometry(), Points, ESlateDrawEffect::None,
				OutlineColor * Tint, true, Thickness + OutlineThickness * 2.f);
		}
		FSlateDrawElement::MakeLines(OutDrawElements, BaseLayer + 2,
			AllottedGeometry.ToPaintGeometry(), Points, ESlateDrawEffect::None,
			CrosshairColor * Tint, true, Thickness);
	}
	return BaseLayer + 2;
}
