#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GamePlay/Combat/Armor/NF_ArmorTypes.h"
#include "Data/Enum/NF_ItemTypes.h"
#include "NF_ArmorDataAsset.generated.h"

UCLASS()
class PROJECT_NF_API UNF_ArmorDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 Cost = 0;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	ENF_ItemType ItemType = ENF_ItemType::Armor;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Armor")
	ENF_ArmorSlot ArmorSlot = ENF_ArmorSlot::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Armor")
	float BaseDefence = 0.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Armor")
	float AdditionalDefence = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Appearance")
	TObjectPtr<class UStaticMesh> ArmorMesh;
};
