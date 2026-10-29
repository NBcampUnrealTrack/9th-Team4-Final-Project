#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NF_WeaponTypes.h"
#include "Data/Enum/NF_ItemTypes.h"
#include "GamePlay/BuildSystem/Core/NF_BuildItemSource.h"
#include "NF_WeaponDataAsset.generated.h"

UENUM(BlueprintType)
enum class ENF_FireMode : uint8
{
	None,
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
class PROJECT_NF_API UNF_WeaponDataAsset : public UDataAsset, public INF_BuildItemSource
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Build")
	TSubclassOf<ANF_WeaponBase> WeaponClass;

	virtual bool GetBuildItemData_Implementation(FNF_BuildItemData& OutData) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 Cost = 0;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	ENF_ItemType ItemType = ENF_ItemType::Weapon;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	ENF_WeaponType WeaponType = ENF_WeaponType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	ENF_FireMode FireMode = ENF_FireMode::None;

	// 무기 메시에서 투사체가 발사되는 소켓.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	FName MuzzleSocketName = TEXT("Muzzle");

	// 무기별 발사 사운드. 재생할 에셋이 없으면 None으로 둔다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FireEffects")
	TObjectPtr<class USoundBase> FireSound = nullptr;

	// MuzzleSocketName 소켓에서 재생할 Cascade 머즐 플래시. 일회성 파티클을 지정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FireEffects")
	TObjectPtr<class UParticleSystem> MuzzleFlash = nullptr;

	// 무기별 발사 카메라 흔들림 클래스. 사용하지 않으면 None으로 둔다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FireEffects")
	TSubclassOf<class UNF_WeaponCameraShakeBase> FireCameraShakeClass;

	// 스코프 카메라를 부착할 무기 메시 소켓.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zoom")
	FName ScopeSocketName = TEXT("Scope");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zoom")
	ENF_ZoomMode ZoomMode = ENF_ZoomMode::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zoom", meta=(ClampMin="5.0", ClampMax="170.0", EditCondition="ZoomMode != ENF_ZoomMode::None"))
	float ZoomFieldOfView = 60.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zoom", meta=(ClampMin="0.0", EditCondition="ZoomMode != ENF_ZoomMode::None"))
	float ZoomInterpSpeed = 12.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Zoom", meta=(ClampMin="0.01", ClampMax="1.0", EditCondition="ZoomMode != ENF_ZoomMode::None"))
	float ZoomSensitivityMultiplier = 0.7f;

	// 스코프 시 캐릭터, 무기 메시 숨김 여부
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
	
	// 카메라 조준점 탐색 거리.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile",
		meta=(ClampMin="1.0", Units="cm"))
	float AimTraceDistance = 100000.f;

	// 카메라 조준점 탐색용 스피어 트레이스 반지름. 0이면 라인 트레이스와 동일하게 동작한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile",
		meta=(ClampMin="0.0", Units="cm"))
	float AimTraceRadius = 0.f;
};
