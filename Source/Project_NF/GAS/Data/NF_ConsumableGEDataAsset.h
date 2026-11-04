#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NF_ConsumableGEDataAsset.generated.h"

class UGameplayEffect;

UCLASS(BlueprintType)
class PROJECT_NF_API UNF_ConsumableGEDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	/* 아래 형식으로 GE 클래스 참조해 주시면 됩니다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay Effects|Consumable")
	TSubclassOf<UGameplayEffect> @@@EffectClass;
	 */
};
