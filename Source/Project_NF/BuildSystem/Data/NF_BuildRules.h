#pragma once

#include "CoreMinimal.h"
#include "BuildSystem/Core/NF_BuildTypes.h"
#include "Engine/DataAsset.h"
#include "NF_BuildRules.generated.h"

class UNF_BuildItemDefinitionDATAASSET;

UCLASS(BlueprintType)
class PROJECT_NF_API UNF_BuildRules : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Build")
	TArray<TObjectPtr<UNF_BuildItemDefinitionDATAASSET>> Catalog;

	/** Index 0 = level 1. Missing levels fail closed; no invented balance curve. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Build")
	TArray<int32> CostLimitsByLevel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Build", meta = (ClampMin = "1", ClampMax = "8"))
	int32 WeaponSlots = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Build", meta = (ClampMin = "0", ClampMax = "8"))
	int32 ArmorSlots = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Build", meta = (ClampMin = "0", ClampMax = "8"))
	int32 ActiveSkillSlots = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Build", meta = (ClampMin = "0", ClampMax = "8"))
	int32 PassiveSkillSlots = 1;

	int32 GetSlotCount(ENF_BuildItemType ItemType) const;
	bool TryGetCostLimit(int32 PlayerLevel, int32& OutCostLimit) const;
};
