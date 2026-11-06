#include "GamePlay/Combat/Weapons/NF_WeaponComponentConfigDataAsset.h"

#include "GAS/Abilities/NFGA_WeaponFire.h"
#include "GAS/Abilities/NFGA_WeaponZoom.h"
#include "GAS/Abilities/NFGA_WeaponReload.h"

UNF_WeaponComponentConfigDataAsset::UNF_WeaponComponentConfigDataAsset()
{
	ARSocketName = TEXT("AR_Socket");
	PistolSocketName = TEXT("Pistol_Socket");
	SGSocketName = TEXT("SG_Socket");
	SMGSocketName = TEXT("SMG_Socket");
	SRSocketName = TEXT("SR_Socket");

	FireAbilityClass = UNFGA_WeaponFire::StaticClass();
	ZoomAbilityClass = UNFGA_WeaponZoom::StaticClass();
	ReloadAbilityClass = UNFGA_WeaponReload::StaticClass();
}
