#include "UI/ViewModel/Status/NF_WeaponStatusViewModel.h"

void UNF_WeaponStatusViewModel::OnInitializeViewModel()
{
	Super::OnInitializeViewModel();
	ClearWeapon();
}

void UNF_WeaponStatusViewModel::OnViewModelContextChanged()
{
	Super::OnViewModelContextChanged();
	ClearWeapon();
}

void UNF_WeaponStatusViewModel::OnDeinitializeViewModel()
{
	ClearWeapon();
	Super::OnDeinitializeViewModel();
}

void UNF_WeaponStatusViewModel::UpdateWeapon(FText NewWeaponName, int32 NewMagazineAmmo, int32 NewReserveAmmo)
{
	UE_MVVM_SET_PROPERTY_VALUE(WeaponName, NewWeaponName);
	UE_MVVM_SET_PROPERTY_VALUE(bHasWeapon, true);
	UpdateAmmo(NewMagazineAmmo, NewReserveAmmo);
}

void UNF_WeaponStatusViewModel::UpdateAmmo(int32 NewMagazineAmmo, int32 NewReserveAmmo)
{
	if (!bHasWeapon) return;
	
	UE_MVVM_SET_PROPERTY_VALUE(MagazineAmmo, FMath::Max(0, NewMagazineAmmo));
	UE_MVVM_SET_PROPERTY_VALUE(ReserveAmmo, FMath::Max(0, NewReserveAmmo));
}

void UNF_WeaponStatusViewModel::ClearWeapon()
{
	UE_MVVM_SET_PROPERTY_VALUE(bHasWeapon, false);
	UE_MVVM_SET_PROPERTY_VALUE(WeaponName, NSLOCTEXT("NFHUD", "NoWeapon", "무기 없음"));
	UE_MVVM_SET_PROPERTY_VALUE(MagazineAmmo, 0);
	UE_MVVM_SET_PROPERTY_VALUE(ReserveAmmo, 0);
}
