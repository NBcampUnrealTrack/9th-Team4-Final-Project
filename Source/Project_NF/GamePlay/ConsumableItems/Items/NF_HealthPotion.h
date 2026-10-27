#pragma once

#include "CoreMinimal.h"
#include "GamePlay/ConsumableItems/NF_ConsumableItem.h"
#include "NF_HealthPotion.generated.h"

// @Owner: 이정준 (구현: Codex) | 2026-10-27
UCLASS(Blueprintable)
class PROJECT_NF_API ANF_HealthPotion : public ANF_ConsumableItem
{
	GENERATED_BODY()
public:
	virtual bool TryUse_Implementation(AActor* User) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Consumable|Health Potion", meta = (ClampMin = "0.001", ToolTip = "한 번 사용할 때 회복할 체력입니다. 기본 30이며 최대 체력을 넘지 않습니다. 최대 체력이거나 사망/기절 상태이면 사용하지 않습니다."))
	float HealAmount = 30.f;
};
