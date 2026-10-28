#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "GamePlay/Combat/Armor/NF_ArmorTypes.h"
#include "NF_ArmorComponent.generated.h"

class UNF_ArmorComponentConfigDataAsset;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECT_NF_API UNF_ArmorComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UNF_ArmorComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Armor")
	bool EquipArmor(class ANF_ArmorBase* NewArmor);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Armor")
	ANF_ArmorBase* UnequipArmor(ENF_ArmorSlot Slot);

	UFUNCTION(BlueprintPure, Category="Armor")
	ANF_ArmorBase* GetEquippedArmor(ENF_ArmorSlot Slot) const;

	UFUNCTION(BlueprintPure, Category="Armor|Hit Regions")
	ENF_ArmorSlot ResolveHitRegion(FName BoneName) const;

	UFUNCTION(BlueprintPure, Category="Armor|Hit Regions")
	float GetAdditionalDefenseForSlot(ENF_ArmorSlot Slot) const;

	UFUNCTION(BlueprintPure, Category="Armor")
	float GetTotalBaseDefence() const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Armor")
	void RefreshArmorDefence();

	void ClearArmorDefence();
	
	class USkeletalMeshComponent* GetHitMesh() const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Armor")
	TObjectPtr<UNF_ArmorComponentConfigDataAsset> Config;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category="Armor")
	TArray<TObjectPtr<class ANF_ArmorBase>> EquippedArmors;

private:
	void ConfigureProjectileCollision();
	FName GetSocketName(ENF_ArmorSlot Slot) const;
	class ANF_ArmorBase* UnequipArmorInternal(ENF_ArmorSlot Slot);
	bool bChangingArmor = false;
	TWeakObjectPtr<class UAbilitySystemComponent> DefenceASC;
	FActiveGameplayEffectHandle DefenceEffectHandle;
};
