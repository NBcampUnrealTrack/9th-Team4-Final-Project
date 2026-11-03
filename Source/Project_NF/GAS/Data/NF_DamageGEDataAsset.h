#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NF_DamageGEDataAsset.generated.h"

class UGameplayEffect;

UCLASS(BlueprintType)
class PROJECT_NF_API UNF_DamageGEDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay Effects|Damage")
	TSubclassOf<UGameplayEffect> CommonDamageEffectClass;
};
