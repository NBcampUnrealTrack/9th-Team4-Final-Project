#include "UI/Widget/NF_SkillSlotWidget.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UNF_SkillSlotWidget::SetSkill(FText SkillName, UTexture2D* Icon, FText Shortcut, bool bPassive)
{
	bIsPassive = bPassive;
	bIsEquipped = true;

	if (TXT_SkillName)
	{
		TXT_SkillName->SetText(SkillName);
	}
	if (IMG_Icon)
	{
		IMG_Icon->SetBrushFromTexture(Icon);
		if (Icon)
		{
			IMG_Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			IMG_Icon->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	if (TXT_Shortcut)
	{
		TXT_Shortcut->SetText(Shortcut);
		if (bIsPassive)
		{
			TXT_Shortcut->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			TXT_Shortcut->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}

	// 장착 정보 변경 시에만 호출. 실제 쿨타임과 사용 가능 여부는 데이터 제공자가 이어서 전달한다.
	SetCooldown(0.0f, 0.0f);
	SetCanUse(false);
}

void UNF_SkillSlotWidget::ClearSkill()
{
	bIsEquipped = false;
	bIsPassive = false;

	if (TXT_SkillName)
	{
		TXT_SkillName->SetText(FText::GetEmpty());
	}
	if (IMG_Icon)
	{
		IMG_Icon->SetBrushFromTexture(nullptr);
		IMG_Icon->SetVisibility(ESlateVisibility::Hidden);
	}
	if (TXT_Shortcut)
	{
		TXT_Shortcut->SetText(FText::GetEmpty());
		TXT_Shortcut->SetVisibility(ESlateVisibility::Collapsed);
	}

	SetCooldown(0.0f, 0.0f);
	SetCanUse(false);
}

void UNF_SkillSlotWidget::SetCooldown(float RemainingSeconds, float DurationSeconds)
{
	const float Remaining = FMath::Max(0.0f, RemainingSeconds);
	const float Duration = FMath::Max(0.0f, DurationSeconds);
	const bool bShowCooldown = bIsEquipped && !bIsPassive && Remaining > 0.0f;

	if (PB_Cooldown)
	{
		if (bShowCooldown && Duration > 0.0f)
		{
			PB_Cooldown->SetPercent(FMath::Clamp(Remaining / Duration, 0.0f, 1.0f));
		}
		else
		{
			PB_Cooldown->SetPercent(0.0f);
		}
		
		if (bShowCooldown)
		{
			PB_Cooldown->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			PB_Cooldown->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	if (TXT_Cooldown)
	{
		if (bShowCooldown)
		{
			TXT_Cooldown->SetText(FText::AsNumber(FMath::CeilToInt(Remaining)));
			TXT_Cooldown->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			TXT_Cooldown->SetText(FText::GetEmpty());
			TXT_Cooldown->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UNF_SkillSlotWidget::SetCanUse(bool bCanUse)
{
	const bool bBright = bIsEquipped && (bIsPassive || bCanUse);
	if (IMG_Icon)
	{
		if (bBright)
		{
			IMG_Icon->SetOpacity(1.0f);
		}
		else
		{
			IMG_Icon->SetOpacity(0.35f);
		}
	}
}
