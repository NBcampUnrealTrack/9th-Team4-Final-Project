#pragma once

#include "CoreMinimal.h"
#include "GamePlay/BuildSystem/Core/NF_BuildTypes.h"
#include "Engine/DataAsset.h"
#include "NF_BuildRules.generated.h"

/** @Owner: 황준현 | 2026-10-26. 상점 식별자와 표시 이름만 소유한다. */
UCLASS(BlueprintType)
class PROJECT_NF_API UNF_BuildItemDefinitionDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Build")
	FName ItemId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Build")
	FText DisplayName;
};

/** @Owner: 황준현 | 2026-11-04. 공통 시작 설정. 런타임 플레이어 상태는 컴포넌트가 보관한다. */
UCLASS(BlueprintType)
class PROJECT_NF_API UNF_BuildInitialSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Build", meta = (ClampMin = "0"))
	int32 InitialCostLimit = 10;
};

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

	UNF_BuildItemDefinitionDataAsset* FindItemDefinition(FName ItemId) const;

	TArray<FNF_BuildCatalogItem> ReadCatalog() const;

	bool ResolveItemData(FName ItemId, FNF_BuildItemData& OutData) const;

	/** Index 0 = level 1. Missing levels fail closed; no invented balance curve. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Build")
	TArray<int32> CostLimitsByLevel;

	// @Owner: 황준현 | 2026-11-05. 슬롯 구성은 무기·방어구·입력 코드가 전제하는 고정값이다.
	// 에디터에서 바꿀 수 없게 표시만 하고, 이전에 다른 값으로 저장된 에셋은 PostLoad에서 고정값으로 되돌린다.
	static constexpr int32 FixedWeaponSlots = 1;
	static constexpr int32 FixedArmorSlots = 3;
	static constexpr int32 FixedActiveSkillSlots = 2;
	static constexpr int32 FixedPassiveSkillSlots = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Build|Slots")
	int32 WeaponSlots = FixedWeaponSlots;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Build|Slots")
	int32 ArmorSlots = FixedArmorSlots;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Build|Slots")
	int32 ActiveSkillSlots = FixedActiveSkillSlots;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Build|Slots")
	int32 PassiveSkillSlots = FixedPassiveSkillSlots;

	bool HasFixedSlotCounts() const;

	int32 GetSlotCount(ENF_ItemType ItemType) const;
	bool TryGetCostLimit(int32 PlayerLevel, int32& OutCostLimit) const;

	// @Owner: 황준현 | 2026-10-19
	// 빌드 제한시간 종료 시 미준비 플레이어에게 적용할 기본 구성.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Build|Fallback")
	TArray<FNF_BuildSelection> FallbackBuild;

	virtual void PostLoad() override;

#if WITH_EDITOR
	/** 저장·데이터 검증 시 카탈로그와 기본 빌드 설정 오류를 알린다. */
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
