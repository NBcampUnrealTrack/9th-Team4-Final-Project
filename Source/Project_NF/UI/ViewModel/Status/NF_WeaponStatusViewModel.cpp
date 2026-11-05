#include "UI/ViewModel/Status/NF_WeaponStatusViewModel.h"

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
	BindWeaponComponent();
}

void UNF_WeaponStatusViewModel::OnDeinitializeViewModel()
{
	UnbindWeaponComponent();
	Super::OnDeinitializeViewModel();
}

void UNF_WeaponStatusViewModel::BindWeaponComponent()
{
	UNF_WeaponComponent* NewComponent = GetViewModelContext().WeaponComponent.Get();

	// 같은 유효한 컴포넌트면 기존 구독을 유지한다.
	if (IsValid(NewComponent) && BoundWeaponComponent.Get() == NewComponent)
	{
		return;
	}

	// 이전 컴포넌트와 무기의 구독을 해제하고 표시 정보를 비운다.
	UnbindWeaponComponent();
	if (!IsValid(NewComponent))
	{
		return;
	}

	BoundWeaponComponent = NewComponent;
	NewComponent->OnEquippedWeaponChanged.AddUObject(this, &ThisClass::HandleEquippedWeaponChanged);
	// HUD 생성 전에 이미 장착된 무기도 현재값으로 복원한다.
	SetObservedWeapon(NewComponent->GetEquippedWeapon());
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
	if (UE_MVVM_SET_PROPERTY_VALUE(MagazineAmmo, FMath::Max(0, Weapon->GetCurrentAmmo())))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetMagazineAmmoText);
	}
	if (UE_MVVM_SET_PROPERTY_VALUE(MagazineSize, FMath::Max(0, Data->MagazineSize)))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetMagazineSizeText);
	}
	UE_MVVM_SET_PROPERTY_VALUE(bHasWeapon, true);
}

void UNF_WeaponStatusViewModel::ClearWeapon()
{
	UE_MVVM_SET_PROPERTY_VALUE(bHasWeapon, false);
	UE_MVVM_SET_PROPERTY_VALUE(WeaponName, NSLOCTEXT("NFHUD", "NoWeapon", "무기 없음"));
	if (UE_MVVM_SET_PROPERTY_VALUE(MagazineAmmo, 0))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetMagazineAmmoText);
	}
	if (UE_MVVM_SET_PROPERTY_VALUE(MagazineSize, 0))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetMagazineSizeText);
	}
}

FText UNF_WeaponStatusViewModel::GetMagazineAmmoText() const
{
	return FText::AsNumber(FMath::Max(0, MagazineAmmo));
}

FText UNF_WeaponStatusViewModel::GetMagazineSizeText() const
{
	return FText::AsNumber(FMath::Max(0, MagazineSize));
}
