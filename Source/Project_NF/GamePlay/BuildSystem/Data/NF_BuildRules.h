#pragma once

#include "CoreMinimal.h"
#include "GamePlay/BuildSystem/Core/NF_BuildTypes.h"
#include "GamePlay/BuildSystem/Core/NF_BuildItemSource.h"
#include "Engine/DataAsset.h"
#include "NF_BuildRules.generated.h"

class UNF_BuildItemDefinitionDataAsset;

UCLASS(BlueprintType)
class PROJECT_NF_API UNF_BuildRules : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Build")
	TArray<TObjectPtr<UNF_BuildItemDefinitionDataAsset>> Catalog;

	/** @Owner: 황준현 | 2026-10-26. ItemId → 각 파트의 원본 DA. 코스트/종류를 복사하지 않는다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Build",
		meta = (MustImplement = "/Script/Project_NF.NF_BuildItemSource"))
	TMap<FName, TObjectPtr<UDataAsset>> ItemSources;

	bool ResolveItemData(FName ItemId, FNF_BuildItemData& OutData) const;

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

	int32 GetSlotCount(ENF_ItemType ItemType) const;
	bool TryGetCostLimit(int32 PlayerLevel, int32& OutCostLimit) const;

	// @Owner: 황준현 | 2026-10-19
	// 빌드 제한시간 종료 시 미준비 플레이어에게 적용할 기본 구성.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Build|Fallback")
	TArray<FNF_BuildSelection> FallbackBuild;
};
