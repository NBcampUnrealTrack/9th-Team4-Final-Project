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

	UFUNCTION(BlueprintPure, Category="Armor")
	ENF_ArmorSlot GetArmorSlot() const { return ArmorSlot; }

	UFUNCTION(BlueprintPure, Category="Armor")
	class UStaticMeshComponent* GetArmorMesh() const { return ArmorMesh; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Armor")
	ENF_ArmorSlot ArmorSlot = ENF_ArmorSlot::Head;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Armor")
	TObjectPtr<class UStaticMeshComponent> ArmorMesh;
};
