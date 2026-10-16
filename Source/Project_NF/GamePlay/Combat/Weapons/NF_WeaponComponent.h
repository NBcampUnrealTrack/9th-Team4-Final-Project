#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "NF_WeaponComponent.generated.h"

class ANF_WeaponBase;
class UAbilitySystemComponent;
class UNF_GA_WeaponFire;
class UNF_GA_WeaponZoom;
class UCameraComponent;
class UUserWidget;
class APawn;
class AController;
class UMeshComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECT_NF_API UNF_WeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UNF_WeaponComponent();
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	bool IsFireInputHeld() const { return bFireInputHeld; }

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void StartFire();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void StopFire();

	/** Bind to Started only; do not bind button release to StopZoom. */
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

	/** Server only. Replaces the current weapon without destroying it; save it before calling if needed. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Weapon")
	bool EquipWeapon(ANF_WeaponBase* NewWeapon);

	/** Server only. Stops combat and returns the detached weapon. Caller owns disposal/rollback. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Weapon")
	ANF_WeaponBase* UnequipWeapon();

	UFUNCTION(BlueprintPure, Category="Weapon")
	ANF_WeaponBase* GetEquippedWeapon() const { return EquippedWeapon; }

protected:
	// 기본값으로 BP_Weapon_Pistol을 지정
	UPROPERTY(EditDefaultsOnly, Category="Weapon")
	TSubclassOf<ANF_WeaponBase> DefaultWeaponClass;

	UPROPERTY(EditDefaultsOnly, Category="Weapon")
	FName WeaponSocketName = TEXT("WeaponSocket_R");

	UPROPERTY(ReplicatedUsing=OnRep_EquippedWeapon, VisibleInstanceOnly, BlueprintReadOnly, Category="Weapon")
	TObjectPtr<ANF_WeaponBase> EquippedWeapon;

	UPROPERTY(EditDefaultsOnly, Category="Weapon|Abilities")
	TSubclassOf<UNF_GA_WeaponFire> FireAbilityClass;

	UPROPERTY(EditDefaultsOnly, Category="Weapon|Abilities")
	TSubclassOf<UNF_GA_WeaponZoom> ZoomAbilityClass;

private:
	friend class UNF_GA_WeaponZoom;
	bool EnsureZoomAbility();
	void SetZoomActive(bool bActive);
	void RestoreZoomCamera();
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

	void RefreshCrosshair();
	void RemoveCrosshair();

	UFUNCTION()
	void OnRep_EquippedWeapon();

	UFUNCTION()
	void OnPawnControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> CrosshairWidget;

	ANF_WeaponBase* UnequipWeaponInternal();
	bool bChangingWeapon = false;
	bool EnsureFireAbility();
	bool bFireInputHeld = false;
	TWeakObjectPtr<UAbilitySystemComponent> FireASC;
	FGameplayAbilitySpecHandle FireAbilityHandle;

	UFUNCTION(Server, Reliable)
	void ServerStartFire();
	UFUNCTION(Server, Reliable)
	void ServerStopFire();
	UFUNCTION(Server, Reliable)
	void ServerReload();
};
