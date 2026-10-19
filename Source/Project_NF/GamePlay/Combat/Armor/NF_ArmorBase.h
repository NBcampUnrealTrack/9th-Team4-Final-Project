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

	UFUNCTION(BlueprintPure, Category="Armor")
	const class UNF_ArmorDataAsset* GetArmorData() const { return ArmorData; }

	UFUNCTION(BlueprintPure, Category="Armor")
	ENF_ArmorSlot GetArmorSlot() const;

	UFUNCTION(BlueprintPure, Category="Armor")
	float GetAdditionalDefense() const;

	UFUNCTION(BlueprintPure, Category="Armor")
	class UStaticMeshComponent* GetArmorMesh() const { return ArmorMesh; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Armor")
	TObjectPtr<class UNF_ArmorDataAsset> ArmorData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Armor")
	TObjectPtr<class UStaticMeshComponent> ArmorMesh;

private:
	void ApplyArmorAppearance();
};
