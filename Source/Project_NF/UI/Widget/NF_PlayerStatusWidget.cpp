#include "UI/Widget/NF_PlayerStatusWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

namespace
{
	float CalculatePercent(float Current, float Maximum)
	{
		if (Maximum > 0.0f)
		{
			return FMath::Clamp(Current / Maximum, 0.0f, 1.0f);
		}
		return 0.0f;
	}

	FText MakeValueText(float Current, float Maximum)
	{
		return FText::Format(
			NSLOCTEXT("NFHUD", "CurrentMaximum", "{0} / {1}"),
			FText::AsNumber(FMath::RoundToInt(Current)),
			FText::AsNumber(FMath::RoundToInt(Maximum)));
	}
}

void UNF_PlayerStatusWidget::SetHealth(float Current, float Maximum)
{
	Maximum = FMath::Max(0.0f, Maximum);
	Current = FMath::Clamp(Current, 0.0f, Maximum);

	if (PB_Health)
	{
		PB_Health->SetPercent(CalculatePercent(Current, Maximum));
	}

	if (TXT_Health)
	{
		TXT_Health->SetText(MakeValueText(Current, Maximum));
	}
}

void UNF_PlayerStatusWidget::SetShield(float Current, float Maximum)
{
	Maximum = FMath::Max(0.0f, Maximum);
	Current = FMath::Clamp(Current, 0.0f, Maximum);

	if (PB_Shield)
	{
		PB_Shield->SetPercent(CalculatePercent(Current, Maximum));
	}

	if (TXT_Shield)
	{
		TXT_Shield->SetText(MakeValueText(Current, Maximum));
	}
}

void UNF_PlayerStatusWidget::SetLevel(int32 NewLevel)
{
	if (TXT_Level)
	{
		TXT_Level->SetText(FText::Format(NSLOCTEXT("NFHUD", "Level", "Lv. {0}"),FText::AsNumber(FMath::Max(1, NewLevel))));
	}
}

void UNF_PlayerStatusWidget::SetExperience(float Current, float Required)
{
	Required = FMath::Max(0.0f, Required);
	Current = FMath::Max(0.0f, Current);

	if (PB_Experience)
	{
		PB_Experience->SetPercent(CalculatePercent(Current, Required));
	}

	if (TXT_Experience)
	{
		TXT_Experience->SetText(MakeValueText(Current, Required));
	}
}