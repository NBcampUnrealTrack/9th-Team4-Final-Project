#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NF_WeaponComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECT_NF_API UNF_WeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UNF_WeaponComponent();
	
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void StartFire();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void StopFire();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void Reload();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	bool EquipWeapon(ANF_WeaponBase* NewWeapon);

	UFUNCTION(BlueprintPure, Category="Weapon")
	ANF_WeaponBase* GetEquippedWeapon() const { return EquippedWeapon; }

protected:
	// 기본값으로 BP_Weapon_Pistol을 지정
	UPROPERTY(EditDefaultsOnly, Category="Weapon")
	TSubclassOf<ANF_WeaponBase> DefaultWeaponClass;

	UPROPERTY(EditDefaultsOnly, Category="Weapon")
	FName WeaponSocketName = TEXT("WeaponSocket");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	TObjectPtr<ANF_WeaponBase> EquippedWeapon;
};
