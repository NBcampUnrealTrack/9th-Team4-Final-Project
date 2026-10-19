#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GamePlay/Combat/Armor/NF_ArmorTypes.h"
#include "NF_ArmorBase.generated.h"

UCLASS()
class PROJECT_NF_API ANF_ArmorBase : public AActor
{
	GENERATED_BODY()
	
public:	
	ANF_ArmorBase();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server only. Tier changes must retain the initially configured part. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Armor")
	bool SetArmorData(class UNF_ArmorDataAsset* NewData);

	UFUNCTION(BlueprintPure, Category="Armor")
	const class UNF_ArmorDataAsset* GetArmorData() const { return ArmorData; }

	UFUNCTION(BlueprintPure, Category="Armor")
	ENF_ArmorSlot GetArmorSlot() const;

	UFUNCTION(BlueprintPure, Category="Armor")
	float GetAdditionalDefense() const;

	UFUNCTION(BlueprintPure, Category="Armor")
	class UStaticMeshComponent* GetArmorMesh() const { return ArmorMesh; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing=OnRep_ArmorData, Category="Armor")
	TObjectPtr<class UNF_ArmorDataAsset> ArmorData;

	/** Logical armor is invisible by default, even when the data asset contains a mesh. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Armor")
	bool bShowArmorAppearance = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Armor")
	TObjectPtr<class UStaticMeshComponent> ArmorMesh;

private:
	UFUNCTION()
	void OnRep_ArmorData();
	void ApplyArmorAppearance();
};
