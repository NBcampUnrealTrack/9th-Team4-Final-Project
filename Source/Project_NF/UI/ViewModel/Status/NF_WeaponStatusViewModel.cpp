#include "UI/ViewModel/Status/NF_WeaponStatusViewModel.h"

#include "GameFramework/Pawn.h"
#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"
#include "GamePlay/Combat/Weapons/NF_WeaponComponent.h"
#include "GamePlay/Combat/Weapons/NF_WeaponDataAsset.h"

namespace
{
	FText GetWeaponTypeText(ENF_WeaponType WeaponType)
	{
		switch (WeaponType)
		{
		case ENF_WeaponType::AR:
			return NSLOCTEXT("NFHUD", "WeaponAR", "돌격소총");
		case ENF_WeaponType::Pistol:
			return NSLOCTEXT("NFHUD", "WeaponPistol", "권총");
		case ENF_WeaponType::SG:
			return NSLOCTEXT("NFHUD", "WeaponSG", "산탄총");
		case ENF_WeaponType::SMG:
			return NSLOCTEXT("NFHUD", "WeaponSMG", "기관단총");
		case ENF_WeaponType::SR:
			return NSLOCTEXT("NFHUD", "WeaponSR", "저격소총");
		default:
			return NSLOCTEXT("NFHUD", "WeaponUnknown", "무기");
		}
	}
}

void UNF_WeaponStatusViewModel::OnInitializeViewModel()
{
	Super::OnInitializeViewModel();
	BindWeaponComponent();
}

void UNF_WeaponStatusViewModel::OnViewModelContextChanged()
{
	Super::OnViewModelContextChanged();
	UnbindWeaponComponent();
	BindWeaponComponent();
}

void UNF_WeaponStatusViewModel::OnDeinitializeViewModel()
{
	UnbindWeaponComponent();
	Super::OnDeinitializeViewModel();
}

void UNF_WeaponStatusViewModel::BindWeaponComponent()
{
	APawn* Pawn = GetViewModelContext().Pawn.Get();
	if (!IsValid(Pawn))
	{
		ClearWeapon();
		return;
	}

	UNF_WeaponComponent* Component = Pawn->FindComponentByClass<UNF_WeaponComponent>();
	if (!IsValid(Component))
	{
		ClearWeapon();
		return;
	}

	BoundWeaponComponent = Component;
	Component->OnEquippedWeaponChanged.AddUObject(this, &ThisClass::HandleEquippedWeaponChanged);
	// HUD 생성 전에 이미 장착된 무기도 현재값으로 복원한다.
	SetObservedWeapon(Component->GetEquippedWeapon());
}

void UNF_WeaponStatusViewModel::UnbindWeaponComponent()
{
	UNF_WeaponComponent* Component = BoundWeaponComponent.Get();
	if (IsValid(Component))
	{
		Component->OnEquippedWeaponChanged.RemoveAll(this);
	}

	BoundWeaponComponent.Reset();
	UnbindWeapon();
	ClearWeapon();
}

void UNF_WeaponStatusViewModel::SetObservedWeapon(ANF_WeaponBase* NewWeapon)
{
	UnbindWeapon();
	if (!IsValid(NewWeapon))
	{
		ClearWeapon();
		return;
	}

	BoundWeapon = NewWeapon;
	NewWeapon->OnCurrentAmmoChanged.AddUObject(this, &ThisClass::HandleCurrentAmmoChanged);
	RefreshWeapon();
}

void UNF_WeaponStatusViewModel::UnbindWeapon()
{
	ANF_WeaponBase* Weapon = BoundWeapon.Get();
	if (IsValid(Weapon))
	{
		Weapon->OnCurrentAmmoChanged.RemoveAll(this);
	}

	BoundWeapon.Reset();
}

void UNF_WeaponStatusViewModel::HandleEquippedWeaponChanged(ANF_WeaponBase* NewWeapon)
{
	SetObservedWeapon(NewWeapon);
}

void UNF_WeaponStatusViewModel::HandleCurrentAmmoChanged(int32)
{
	RefreshWeapon();
}

void UNF_WeaponStatusViewModel::RefreshWeapon()
{
	ANF_WeaponBase* Weapon = BoundWeapon.Get();
	if (!IsValid(Weapon))
	{
		ClearWeapon();
		return;
	}

	const UNF_WeaponDataAsset* Data = Weapon->GetWeaponData();
	if (!IsValid(Data))
	{
		ClearWeapon();
		return;
	}

	UE_MVVM_SET_PROPERTY_VALUE(WeaponName, GetWeaponTypeText(Data->WeaponType));
	UE_MVVM_SET_PROPERTY_VALUE(MagazineAmmo, FMath::Max(0, Weapon->GetCurrentAmmo()));
	UE_MVVM_SET_PROPERTY_VALUE(MagazineSize, FMath::Max(0, Data->MagazineSize));
	UE_MVVM_SET_PROPERTY_VALUE(bHasWeapon, true);
}

void UNF_WeaponStatusViewModel::ClearWeapon()
{
	UE_MVVM_SET_PROPERTY_VALUE(bHasWeapon, false);
	UE_MVVM_SET_PROPERTY_VALUE(WeaponName, NSLOCTEXT("NFHUD", "NoWeapon", "무기 없음"));
	UE_MVVM_SET_PROPERTY_VALUE(MagazineAmmo, 0);
	UE_MVVM_SET_PROPERTY_VALUE(MagazineSize, 0);
}
