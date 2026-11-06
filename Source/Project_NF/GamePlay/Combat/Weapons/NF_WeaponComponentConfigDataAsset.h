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
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	TSubclassOf<class ANF_WeaponBase> DefaultWeaponClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Abilities")
	TSubclassOf<class UNFGA_WeaponFire> FireAbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Abilities")
	TSubclassOf<class UNFGA_WeaponZoom> ZoomAbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Abilities")
	TSubclassOf<class UNFGA_WeaponReload> ReloadAbilityClass;
};
