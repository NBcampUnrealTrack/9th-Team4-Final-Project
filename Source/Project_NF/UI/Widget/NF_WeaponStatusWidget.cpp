#include "UI/Widget/NF_WeaponStatusWidget.h"

#include "Components/TextBlock.h"

void UNF_WeaponStatusWidget::SetWeaponName(FText NewName)
{
	if (TXT_WeaponName)
	{
		TXT_WeaponName->SetText(NewName);
	}
}

void UNF_WeaponStatusWidget::SetAmmo(int32 MagazineAmmo, int32 ReserveAmmo)
{
	if (TXT_MagazineAmmo)
	{
		TXT_MagazineAmmo->SetText(FText::AsNumber(FMath::Max(0, MagazineAmmo)));
	}
	if (TXT_ReserveAmmo)
	{
		TXT_ReserveAmmo->SetText(FText::AsNumber(FMath::Max(0, ReserveAmmo)));
	}
}

void UNF_WeaponStatusWidget::ClearWeapon()
{
	SetWeaponName(NSLOCTEXT("NFHUD", "NoWeapon", "무기 없음"));
	SetAmmo(0, 0);
}
