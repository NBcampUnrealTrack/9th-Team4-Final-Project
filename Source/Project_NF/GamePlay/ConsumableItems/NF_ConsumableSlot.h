#pragma once

#include "CoreMinimal.h"
#include "NF_ConsumableSlot.generated.h"

class UNF_ConsumableItemDataAsset;

// @Owner: 이정준 (구현: Codex) | 2026-10-26
// 슬롯에는 원본 참조와 수량만 저장한다. 이름/설명/아이콘은 원본 DA에서 조회한다.
USTRUCT(BlueprintType)
struct PROJECT_NF_API FNF_ConsumableSlot
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "NF|Consumable|Slot")
	TObjectPtr<UNF_ConsumableItemDataAsset> ItemData = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "NF|Consumable|Slot")
	int32 Quantity = 0;
};
