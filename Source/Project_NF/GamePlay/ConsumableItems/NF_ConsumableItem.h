#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NF_ConsumableItem.generated.h"

// @Owner: 이정준 (구현: Codex) | 2026-10-27
// DA의 ItemClass가 참조하는 사용 효과. 컴포넌트가 서버에서 잠시 생성하고 사용 후 제거한다.
UCLASS(Abstract, Blueprintable)
class PROJECT_NF_API ANF_ConsumableItem : public AActor
{
	GENERATED_BODY()
public:
	ANF_ConsumableItem();

	// 즉시 실행 규약: 효과가 실제 적용된 경우만 true. 수량 차감은 컴포넌트가 담당한다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Consumable|Use")
	bool TryUse(AActor* User);
	virtual bool TryUse_Implementation(AActor* User);
};
