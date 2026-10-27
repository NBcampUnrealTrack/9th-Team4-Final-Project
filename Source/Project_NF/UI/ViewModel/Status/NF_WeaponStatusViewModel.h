#pragma once

#include "CoreMinimal.h"
#include "UI/ViewModel/Base/NF_ViewModelBase.h"
#include "NF_WeaponStatusViewModel.generated.h"

class ANF_WeaponBase;
class UNF_WeaponComponent;

// 클라이언트 무기 변경 알림을 구독하여 HUD 바인딩에 제공한다.
UCLASS(BlueprintType)
class PROJECT_NF_API UNF_WeaponStatusViewModel : public UNF_ViewModelBase
{
	GENERATED_BODY()

protected:
	virtual void OnInitializeViewModel() override;
	virtual void OnViewModelContextChanged() override;
	virtual void OnDeinitializeViewModel() override;

private:
	void BindWeaponComponent();
	void UnbindWeaponComponent();
	void SetObservedWeapon(ANF_WeaponBase* NewWeapon);
	void UnbindWeapon();
	void HandleEquippedWeaponChanged(ANF_WeaponBase* NewWeapon);
	void HandleCurrentAmmoChanged(int32 NewAmmo);
	void RefreshWeapon();
	void ClearWeapon();

	TWeakObjectPtr<UNF_WeaponComponent> BoundWeaponComponent;
	TWeakObjectPtr<ANF_WeaponBase> BoundWeapon;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "NF|UI|Weapon", meta = (AllowPrivateAccess = "true"))
	FText WeaponName;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "NF|UI|Weapon", meta = (AllowPrivateAccess = "true"))
	int32 MagazineAmmo = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "NF|UI|Weapon", meta = (AllowPrivateAccess = "true"))
	int32 MagazineSize = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "NF|UI|Weapon", meta = (AllowPrivateAccess = "true"))
	bool bHasWeapon = false;
};
