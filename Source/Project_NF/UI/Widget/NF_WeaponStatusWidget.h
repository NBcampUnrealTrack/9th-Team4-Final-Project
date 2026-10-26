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
	void SetAmmo(int32 MagazineAmmo, int32 MagazineSize);

	UFUNCTION(BlueprintCallable, Category = "NF|UI|Weapon")
	void ClearWeapon();

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_WeaponName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_MagazineAmmo;

	// 탄창 크기를 별도 텍스트로 표시할 경우 사용한다.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_MagazineSize;
};
