#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NF_WeaponDataAsset.generated.h"

UENUM(BlueprintType)
enum class ENF_WeaponType : uint8
{
	AR		UMETA(DisplayName = "Assault Rifle"),
	Pistol	UMETA(DisplayName = "Pistol"),	
	SG		UMETA(DisplayName = "Shotgun"),
	SMG		UMETA(DisplayName = "Submachine Gun"),
	SR		UMETA(DisplayName = "Sniper Rifle")
};

UENUM(BlueprintType)
enum class ENF_FireMode : uint8
{
	SemiAutomatic	UMETA(DisplayName="Semi Automatic"),
	Automatic		UMETA(DisplayName="Automatic")
};

UCLASS()
class PROJECT_NF_API UNF_WeaponDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	ENF_WeaponType WeaponType = ENF_WeaponType::Pistol;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	ENF_FireMode FireMode = ENF_FireMode::SemiAutomatic;
	
	// 데미지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	float Damage = 10.f;
	
	// 연사력 (RPM)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	float RoundsPerMinute = 300.f;

	// 탄창 크기
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	int32 MagazineSize = 12;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile")
	TSubclassOf<class ANF_WeaponProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly,	BlueprintReadOnly, Category="Projectile")
	float ProjectileSpeed = 5000.f;

	// 발사 한 번에 나가는 투사체 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile")
	int32 ProjectilesPerShot = 1;

	// 카메라 조준점으로 향하는 발사 방향 기준 퍼짐 반각
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile",
		meta=(ClampMin="0.0", ClampMax="90.0", Units="deg"))
	float SpreadAngleDegrees = 0.f;
};
