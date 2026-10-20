#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NF_WeaponBase.generated.h"

UCLASS()
class PROJECT_NF_API ANF_WeaponBase : public AActor
{
	GENERATED_BODY()
	
public:	
	ANF_WeaponBase();
	
	virtual void OnConstruction(const FTransform& Transform) override;	// 에디터에서 적용
	virtual void PostInitializeComponents() override;					// 게임 실행 시 적용
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void StartFire();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void StopFire();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void Reload();

	/** Cancel without refilling ammo or resetting the fire interval. Safe to call repeatedly. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Weapon")
	void CancelReload();

	UFUNCTION(BlueprintPure, Category="Weapon")
	bool CanReload() const;

	UFUNCTION(BlueprintPure, Category="Weapon")
	bool IsReloading() const { return bIsReloading; }
	
	bool FireProjectile();
	bool CanFire() const;
	float GetFireInterval() const;
	float GetRemainingFireDelay() const;
	class USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMeshComponent; }
	FName GetMuzzleSocketName() const { return MuzzleSocketName; }
	FName GetScopeSocketName() const { return ScopeSocketName; }

	UFUNCTION(BlueprintPure, Category="Weapon")
	int32 GetCurrentAmmo() const { return CurrentAmmo; }

	UFUNCTION(BlueprintPure, Category="Weapon")
	const class UNF_WeaponDataAsset* GetWeaponData() const { return WeaponData; }
	
protected:
	void FinishReload();
	FVector GetFireDirection(const FTransform& MuzzleTransform) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	float AimTraceDistance = 100000.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
	TObjectPtr<class USkeletalMeshComponent> WeaponMeshComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	TObjectPtr<class UNF_WeaponDataAsset> WeaponData;
	
	// 무기 메시에 Muzzle 소켓 생성해야 함
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	FName MuzzleSocketName = TEXT("Muzzle");

	// Weapon mesh socket used only by the scoped camera.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	FName ScopeSocketName = TEXT("Scope");

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category="Weapon")
	int32 CurrentAmmo = 0;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category="Weapon")
	bool bIsReloading = false;

private:
	friend class UNFGA_WeaponReload;
	void ApplyWeaponAppearance();

	double LastFireTime = -1.e30;
};
