#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NF_ConsumableItemDataAsset.generated.h"

class UTexture2D;
class AActor;

// @Owner: 이정준 (구현: Codex) | 2026-10-26
// 소모품 정적 데이터의 원본. 보유 수량이나 슬롯 상태를 저장하지 않는다.
UCLASS(BlueprintType)
class PROJECT_NF_API UNF_ConsumableItemDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Consumable", meta = (ToolTip = "소모품을 구분하는 식별자입니다. 동일한 아이템은 동일한 원본 DA를 사용합니다."))
	FName ItemId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Consumable|Display", meta = (ToolTip = "UI에 표시할 소모품 이름입니다."))
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Consumable|Display", meta = (MultiLine = "true", ToolTip = "UI에 표시할 소모품 설명입니다."))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Consumable|Display", meta = (ToolTip = "UI에 표시할 소모품 아이콘입니다. 미지정을 허용합니다."))
	TObjectPtr<UTexture2D> Icon = nullptr;

	UFUNCTION(BlueprintPure, Category = "NF|Consumable|Display")
	UTexture2D* GetItemIcon() const { return Icon.Get(); }

	// @Owner: 이정준 (구현: Codex) | 2026-10-27
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Consumable", meta = (ToolTip = "사용 효과를 구현한 NF_ConsumableItem 자식 클래스입니다. 체력 물약은 NF_HealthPotion 또는 그 자식 BP를 지정합니다. 슬롯 사용 시 서버에서 잠시 생성해 효과를 실행합니다."))
	TSubclassOf<AActor> ItemClass;
};
