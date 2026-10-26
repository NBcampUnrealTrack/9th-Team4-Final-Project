#include "UI/Widget/NF_TeamMemberWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UNF_TeamMemberWidget::SetMemberName(FText NewName)
{
	if (TXT_PlayerName)
	{
		TXT_PlayerName->SetText(NewName);
	}
}

void UNF_TeamMemberWidget::SetMemberLevel(int32 NewLevel)
{
	if (TXT_Level)
	{
		TXT_Level->SetText(FText::Format(
			NSLOCTEXT("NFHUD", "TeamMemberLevel", "Lv. {0}"),
			FText::AsNumber(FMath::Max(1, NewLevel))));
	}
}

void UNF_TeamMemberWidget::SetHealth(float Current, float Maximum)
{
	if (PB_Health)
	{
		if (Maximum > 0.0f)
		{
			PB_Health->SetPercent(FMath::Clamp(Current / Maximum, 0.0f, 1.0f));
		}
		else
		{
			PB_Health->SetPercent(0.0f);
		}
	}
}

void UNF_TeamMemberWidget::SetShield(float Current, float Maximum)
{
	if (PB_Shield)
	{
		if (Maximum > 0.0f)
		{
			PB_Shield->SetPercent(FMath::Clamp(Current / Maximum, 0.0f, 1.0f));
		}
		else
		{
			PB_Shield->SetPercent(0.0f);
		}
	}
}
