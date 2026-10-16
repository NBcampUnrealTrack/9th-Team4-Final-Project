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

UENUM(BlueprintType)
enum class ENF_ZoomMode : uint8
{
	None,
	Aim,
	Scope
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zoom")
	ENF_ZoomMode ZoomMode = ENF_ZoomMode::Aim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zoom", meta=(ClampMin="5.0", ClampMax="170.0", EditCondition="ZoomMode != ENF_ZoomMode::None"))
	float ZoomFieldOfView = 60.f;

	// Zero applies the target FOV immediately.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zoom", meta=(ClampMin="0.0", EditCondition="ZoomMode != ENF_ZoomMode::None"))
	float ZoomInterpSpeed = 12.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zoom", meta=(ClampMin="0.01", ClampMax="1.0", EditCondition="ZoomMode != ENF_ZoomMode::None"))
	float ZoomSensitivityMultiplier = 0.7f;

	// Hide character/weapon meshes only for the local player while scoped.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zoom", meta=(EditCondition="ZoomMode == ENF_ZoomMode::Scope"))
	bool bHideMeshesWhileScoped = true;

	// Keep the serialized name to preserve existing asset overrides; attachment now uses ScopeSocketName.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zoom", meta=(DisplayName="Attach Scope Camera To Scope Socket", EditCondition="ZoomMode == ENF_ZoomMode::Scope"))
	bool bAttachScopeCameraToMuzzle = true;
	
	// 지정하면 BP의 메시/머티리얼 대신 이 DA의 외형을 적용한다. None이면 기존 BP 설정 유지.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Appearance")
	TObjectPtr<class USkeletalMesh> WeaponMesh;

	// 배열 인덱스 = 메시의 머티리얼 슬롯 인덱스. None 또는 생략한 슬롯은 메시 기본값 사용.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Appearance",
		meta=(EditCondition="WeaponMesh != nullptr"))
	TArray<TObjectPtr<class UMaterialInterface>> WeaponMaterials;

	// 장착 시 로컬 플레이어 화면에 표시할 위젯. None이면 표시하지 않는다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	TSubclassOf<class UUserWidget> CrosshairWidgetClass;
	
	// Replaces the normal crosshair while zooming in Scope mode. None keeps the normal crosshair.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI", meta=(EditCondition="ZoomMode == ENF_ZoomMode::Scope"))
	TSubclassOf<class UUserWidget> ScopeWidgetClass;
	
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
