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
	
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void StartFire();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void StopFire();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void Reload();

	UFUNCTION(BlueprintPure, Category="Weapon")
	int32 GetCurrentAmmo() const { return CurrentAmmo; }

	UFUNCTION(BlueprintPure, Category="Weapon")
	const UNF_WeaponDataAsset* GetWeaponData() const { return WeaponData; }
	
protected:
	void FireOnce();
	void FinishReload();

	float GetFireInterval() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
	TObjectPtr<USkeletalMeshComponent> WeaponMeshComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	TObjectPtr<UNF_WeaponDataAsset> WeaponData;
	
	// 무기 메시에 Muzzle 소켓 생성해야 함
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	FName MuzzleSocketName = TEXT("Muzzle");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	int32 CurrentAmmo = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	bool bIsReloading = false;

private:
	FTimerHandle FireTimerHandle;
	FTimerHandle ReloadTimerHandle;
};
