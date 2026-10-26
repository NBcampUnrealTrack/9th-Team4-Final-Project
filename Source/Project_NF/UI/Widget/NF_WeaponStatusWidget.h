#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NF_WeaponStatusWidget.generated.h"

class UTextBlock;

UCLASS(Abstract, Blueprintable)
class PROJECT_NF_API UNF_WeaponStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "NF|UI|Weapon")
	void SetWeaponName(FText NewName);

	UFUNCTION(BlueprintCallable, Category = "NF|UI|Weapon")
	void SetAmmo(int32 MagazineAmmo, int32 ReserveAmmo);

	UFUNCTION(BlueprintCallable, Category = "NF|UI|Weapon")
	void ClearWeapon();

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_WeaponName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_MagazineAmmo;

	// 예비 탄약을 표시하지 않는 WBP에서는 생략 가능.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_ReserveAmmo;
};
