#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "NF_WeaponTypes.h"
#include "NF_WeaponComponent.generated.h"

class ANF_WeaponBase;
class UNF_WeaponComponentConfigDataAsset;
class UAbilitySystemComponent;
class UNFGA_WeaponFire;
class UNFGA_WeaponZoom;
class UNFGA_WeaponReload;
class UCameraComponent;
class APawn;
class AController;
class UMeshComponent;
class USceneComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;
enum class EVisibilityBasedAnimTickOption : uint8;

DECLARE_MULTICAST_DELEGATE_OneParam(FNF_OnEquippedWeaponChanged, ANF_WeaponBase* /* NewWeapon, nullptr if unequipped */);

DECLARE_MULTICAST_DELEGATE_OneParam(FNF_OnZoomActiveChanged, bool /* bZoomActive */);
DECLARE_MULTICAST_DELEGATE_OneParam(FNF_OnLocalShotFired, ANF_WeaponBase* /* FiredWeapon */);

DECLARE_MULTICAST_DELEGATE_ThreeParams(FNF_OnShotFired, AActor* /* Shooter */,
	ANF_WeaponBase* /* Weapon */, bool /* bLastRound */);

DECLARE_MULTICAST_DELEGATE_FourParams(FNF_OnAttackHit, AActor* /* Attacker */,
	AActor* /* Target */, FVector /* HitLocation */, FGameplayTag /* AttackType */);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECT_NF_API UNF_WeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UNF_WeaponComponent();
	
	FNF_OnEquippedWeaponChanged OnEquippedWeaponChanged;

	// 서버의 줌 상태 변경 및 클라이언트의 복제 수신 시 발생한다.
	FNF_OnZoomActiveChanged OnZoomActiveChanged;

	// 로컬 플레이어의 현재 장착 무기에 대한 발사 Cue에서 발생한다.
	FNF_OnLocalShotFired OnLocalShotFired;
	
	FNF_OnShotFired OnShotFired;
	
	FNF_OnAttackHit OnAttackHit;
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	bool IsFireInputHeld() const { return bFireInputHeld; }

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void StartFire();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void StopFire();
	
	UFUNCTION(BlueprintCallable, Category="Weapon|Zoom")
	void ToggleZoom();

	UFUNCTION(BlueprintCallable, Category="Weapon|Zoom")
	void StartZoom();

	UFUNCTION(BlueprintCallable, Category="Weapon|Zoom")
	void StopZoom();

	UFUNCTION(BlueprintPure, Category="Weapon|Zoom")
	bool IsZooming() const { return bZoomActive; }

	UFUNCTION(BlueprintPure, Category="Weapon|Zoom")
	bool CanZoom() const;

	UFUNCTION(BlueprintPure, Category="Weapon|Zoom")
	float GetZoomSensitivityMultiplier() const;

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void Reload();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Weapon")
	void CancelReload();
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Weapon")
	bool EquipWeapon(ANF_WeaponBase* NewWeapon);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Weapon")
	ANF_WeaponBase* UnequipWeapon();

	UFUNCTION(BlueprintPure, Category="Weapon")
	ANF_WeaponBase* GetEquippedWeapon() const { return EquippedWeapon; }

	// 현재 착용 중인 무기의 타입. 무기 또는 데이터가 없으면 None.
	UFUNCTION(BlueprintPure, Category="Weapon")
	ENF_WeaponType GetEquippedWeaponType() const;

	// 현재 착용 중인 무기가 실제로 부착된 소켓 이름. 미장착 시 NAME_None.
	UFUNCTION(BlueprintPure, Category="Weapon")
	FName GetEquippedWeaponSocketName() const;

	// 현재 착용 중인 무기의 메시 컴포넌트. 미장착 시 nullptr.
	UFUNCTION(BlueprintPure, Category="Weapon")
	UStaticMeshComponent* GetEquippedWeaponMesh() const;

	void RefreshWeaponDataPresentation(ANF_WeaponBase* ChangedWeapon);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	TObjectPtr<UNF_WeaponComponentConfigDataAsset> Config;

	UPROPERTY(ReplicatedUsing=OnRep_EquippedWeapon, VisibleInstanceOnly, BlueprintReadOnly, Category="Weapon")
	TObjectPtr<ANF_WeaponBase> EquippedWeapon;

private:
	friend class UNFGA_WeaponZoom;
	bool EnsureZoomAbility();
	void SetZoomActive(bool bActive);
	void RestoreZoomCamera();
	void UpdateScopeCameraAttachment(bool bAttach);
	bool bScopeCameraAttached = false;
	TWeakObjectPtr<USceneComponent> OriginalCameraParent;
	FName OriginalCameraSocket;
	FTransform OriginalCameraTransform;
	bool bOriginalAbsoluteLocation = false;
	bool bOriginalAbsoluteRotation = false;
	bool bOriginalAbsoluteScale = false;
	bool bOriginalUsePawnControlRotation = false;
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, EVisibilityBasedAnimTickOption> ScopeMeshTickOptions;
	void UpdateScopedMeshes(bool bHide);
	TArray<TWeakObjectPtr<UMeshComponent>> HiddenScopeMeshes;
	TWeakObjectPtr<ANF_WeaponBase> PresentedZoomWeapon;
	float ActiveZoomInterpSpeed = 12.f;
	TWeakObjectPtr<UAbilitySystemComponent> ZoomASC;
	FGameplayAbilitySpecHandle ZoomAbilityHandle;
	TWeakObjectPtr<UCameraComponent> ZoomCamera;
	float UnzoomedFieldOfView = 100.f;

	UPROPERTY(ReplicatedUsing=OnRep_ZoomActive)
	bool bZoomActive = false;

	UFUNCTION()
	void OnRep_ZoomActive();
	UFUNCTION(Server, Reliable)
	void ServerToggleZoom();
	UFUNCTION(Server, Reliable)
	void ServerStartZoom();
	UFUNCTION(Server, Reliable)
	void ServerStopZoom();

	UFUNCTION()
	void OnRep_EquippedWeapon();

	UFUNCTION()
	void OnPawnControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	ANF_WeaponBase* UnequipWeaponInternal();
	bool bChangingWeapon = false;
	bool EnsureFireAbility();
	bool bFireInputHeld = false;
	TWeakObjectPtr<UAbilitySystemComponent> FireASC;
	FGameplayAbilitySpecHandle FireAbilityHandle;
	bool EnsureReloadAbility();
	TWeakObjectPtr<UAbilitySystemComponent> ReloadASC;
	FGameplayAbilitySpecHandle ReloadAbilityHandle;

	UFUNCTION(Server, Reliable)
	void ServerStartFire();
	UFUNCTION(Server, Reliable)
	void ServerStopFire();
	UFUNCTION(Server, Reliable)
	void ServerReload();
};
