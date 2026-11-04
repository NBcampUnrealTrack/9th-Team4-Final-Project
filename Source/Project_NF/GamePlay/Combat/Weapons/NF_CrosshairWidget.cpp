#include "GamePlay/Combat/Weapons/NF_CrosshairWidget.h"

#include "Components/Widget.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"
#include "GamePlay/Combat/Weapons/NF_WeaponComponent.h"

void UNF_CrosshairWidget::SetMaxRadius(float InValue)
{
	MaxRadius = FMath::Max(0.f, InValue);
	UpdateCrosshairLayout();
}

void UNF_CrosshairWidget::UpdateRadius(float InValue)
{
	Radius = FMath::Clamp(InValue, 0.f, 1.f);
	UpdateCrosshairLayout();
}

void UNF_CrosshairWidget::NotifyShotFired()
{
	UpdateRadius(Radius + FMath::Clamp(SpreadPerShot, 0.f, 1.f));
}

void UNF_CrosshairWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	UpdateCrosshairLayout();
}

void UNF_CrosshairWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	UpdateCrosshairLayout();
	UpdateCrosshairVisibility();
}

void UNF_CrosshairWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!IsDesignTime() && Radius > 0.f)
	{
		UpdateRadius(FMath::FInterpConstantTo(Radius, 0.f, InDeltaTime,
			FMath::Max(0.01f, SpreadRecoverySpeed)));
	}
	UpdateCrosshairVisibility();
}

void UNF_CrosshairWidget::UpdateCrosshairLayout()
{
	const float Distance = FMath::Max(0.f, CenterGap)
		+ FMath::Max(0.f, MaxRadius) * FMath::Clamp(Radius, 0.f, 1.f);
	
	if (CrosshairTop)
	{
		CrosshairTop->SetRenderTranslation(FVector2D(0.f, -Distance));
	}
	if (CrosshairBottom)
	{
		CrosshairBottom->SetRenderTranslation(FVector2D(0.f, Distance));
	}
	if (CrosshairLeft)
	{
		CrosshairLeft->SetRenderTranslation(FVector2D(-Distance, 0.f));
	}
	if (CrosshairRight)
	{
		CrosshairRight->SetRenderTranslation(FVector2D(Distance, 0.f));
	}
}

void UNF_CrosshairWidget::UpdateCrosshairVisibility()
{
	if (IsDesignTime())
	{
		return;
	}

	const APlayerController* Controller = GetOwningPlayer();
	const APawn* Pawn = GetOwningPlayerPawn();
	const UNF_WeaponComponent* Weapons = Pawn ? Pawn->FindComponentByClass<UNF_WeaponComponent>() : nullptr;
	const bool bShowCrosshair = Controller && Controller->IsLocalController() && Weapons &&
		IsValid(Weapons->GetEquippedWeapon()) && !Controller->bShowMouseCursor &&
		(!Controller->GetHUD() || Controller->GetHUD()->bShowHUD);
	
	if (UWidget* RootWidget = GetRootWidget())
	{
		RootWidget->SetVisibility(bShowCrosshair
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}
