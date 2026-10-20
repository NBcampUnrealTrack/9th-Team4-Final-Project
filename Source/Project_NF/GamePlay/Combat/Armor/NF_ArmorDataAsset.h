#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GamePlay/Combat/Armor/NF_ArmorTypes.h"
#include "NF_ArmorDataAsset.generated.h"

UCLASS()
class PROJECT_NF_API UNF_ArmorDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Armor")
	ENF_ArmorSlot ArmorSlot = ENF_ArmorSlot::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Armor")
	float AdditionalDefense = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Appearance")
	TObjectPtr<class UStaticMesh> ArmorMesh;
};
