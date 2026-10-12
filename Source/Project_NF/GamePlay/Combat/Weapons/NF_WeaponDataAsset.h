#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NF_WeaponDataAsset.generated.h"

UENUM(BlueprintType)
enum class ENF_WeaponType : uint8
{
	AR,
	Pistol,
	SG,
	SMG,
	SR
};

UCLASS()
class PROJECT_NF_API UNF_WeaponDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	ENF_WeaponType WeaponType = ENF_WeaponType::Pistol;
	
	// 데미지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	float Damage = 10.f;
	
	// 연사력
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	float RoundsPerMinute = 300.f;

	// 탄창 크기
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	int32 MagazineSize = 12;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile")
	TSubclassOf<class ANF_ProjectileBase> ProjectileClass;

	UPROPERTY(EditDefaultsOnly,	BlueprintReadOnly, Category="Projectile")
	float ProjectileSpeed = 5000.f;
};
