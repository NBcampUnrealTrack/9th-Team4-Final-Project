#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NF_WeaponComponentConfigDataAsset.generated.h"

UCLASS(BlueprintType)
class PROJECT_NF_API UNF_WeaponComponentConfigDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UNF_WeaponComponentConfigDataAsset();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
	TSubclassOf<class ANF_WeaponBase> DefaultWeaponClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
	FName WeaponSocketName = TEXT("WeaponSocket_R");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Abilities")
	TSubclassOf<class UNFGA_WeaponFire> FireAbilityClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Abilities")
	TSubclassOf<class UNFGA_WeaponZoom> ZoomAbilityClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Abilities")
	TSubclassOf<class UNFGA_WeaponReload> ReloadAbilityClass;
};
