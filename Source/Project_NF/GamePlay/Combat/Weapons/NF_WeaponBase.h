#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NF_WeaponBase.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FNF_OnCurrentAmmoChanged, int32 /* CurrentAmmo */);

UCLASS()
class PROJECT_NF_API ANF_WeaponBase : public AActor
{
	GENERATED_BODY()
	
public:	
	ANF_WeaponBase();

	/** Server mutations and client replication. Subscribe, then read GetCurrentAmmo(). */
	FNF_OnCurrentAmmoChanged OnCurrentAmmoChanged;
	
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
	FName GetMuzzleSocketName() const;
	FName GetScopeSocketName() const;

	UFUNCTION(BlueprintPure, Category="Weapon")
	int32 GetCurrentAmmo() const { return CurrentAmmo; }

	UFUNCTION(BlueprintPure, Category="Weapon")
	const class UNF_WeaponDataAsset* GetWeaponData() const { return WeaponData; }
	
protected:
	void FinishReload();
	FVector GetFireDirection(const FTransform& MuzzleTransform) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
	TObjectPtr<class USkeletalMeshComponent> WeaponMeshComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	TObjectPtr<class UNF_WeaponDataAsset> WeaponData;
	
	UPROPERTY(ReplicatedUsing=OnRep_CurrentAmmo, VisibleInstanceOnly, BlueprintReadOnly, Category="Weapon")
	int32 CurrentAmmo = 0;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category="Weapon")
	bool bIsReloading = false;

private:
	friend class FNF_WeaponPassivesTest;
	friend class UNFGA_WeaponReload;
	void ApplyWeaponAppearance();
	void SetCurrentAmmo(int32 NewAmmo);

	UFUNCTION()
	void OnRep_CurrentAmmo();

	double LastFireTime = -1.e30;
};
