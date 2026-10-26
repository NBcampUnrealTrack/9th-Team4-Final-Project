#pragma once

#include "CoreMinimal.h"
#include "UI/ViewModel/Base/NF_ViewModelBase.h"
#include "NF_WeaponStatusViewModel.generated.h"

// 클라이언트에 수신한 무기 데이터를 전달받아 HUD 바인딩에 제공한다.
UCLASS(BlueprintType)
class PROJECT_NF_API UNF_WeaponStatusViewModel : public UNF_ViewModelBase
{
	GENERATED_BODY()

public:
	// 무기 장착/교체 시 호출한다.
	UFUNCTION(BlueprintCallable, Category = "NF|UI|Weapon")
	void UpdateWeapon(FText NewWeaponName, int32 NewMagazineAmmo, int32 NewReserveAmmo);

	// 발사/재장전/탄약 획득 시 호출한다.
	UFUNCTION(BlueprintCallable, Category = "NF|UI|Weapon")
	void UpdateAmmo(int32 NewMagazineAmmo, int32 NewReserveAmmo);

	UFUNCTION(BlueprintCallable, Category = "NF|UI|Weapon")
	void ClearWeapon();

protected:
	virtual void OnInitializeViewModel() override;
	virtual void OnViewModelContextChanged() override;
	virtual void OnDeinitializeViewModel() override;

private:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "NF|UI|Weapon", meta = (AllowPrivateAccess = "true"))
	FText WeaponName;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "NF|UI|Weapon", meta = (AllowPrivateAccess = "true"))
	int32 MagazineAmmo = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "NF|UI|Weapon", meta = (AllowPrivateAccess = "true"))
	int32 ReserveAmmo = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "NF|UI|Weapon", meta = (AllowPrivateAccess = "true"))
	bool bHasWeapon = false;
};
