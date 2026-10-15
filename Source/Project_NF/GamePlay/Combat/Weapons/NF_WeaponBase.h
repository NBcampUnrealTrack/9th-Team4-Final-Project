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
	
	virtual void BeginPlay() override;
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
	
	bool FireProjectile();
	bool CanFire() const;
	float GetFireInterval() const;
	float GetRemainingFireDelay() const;

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

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category="Weapon")
	int32 CurrentAmmo = 0;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category="Weapon")
	bool bIsReloading = false;

private:
	double LastFireTime = -1.e30;
	FTimerHandle ReloadTimerHandle;
};
