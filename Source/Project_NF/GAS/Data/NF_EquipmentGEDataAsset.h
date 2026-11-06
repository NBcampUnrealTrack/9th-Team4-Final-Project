#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NF_EquipmentGEDataAsset.generated.h"

class UGameplayEffect;

UCLASS(BlueprintType)
class PROJECT_NF_API UNF_EquipmentGEDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay Effects|Equipment")
	TSubclassOf<UGameplayEffect> NFGE_ArmorDefence;
};
