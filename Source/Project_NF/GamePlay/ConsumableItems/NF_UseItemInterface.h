#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NF_UseItemInterface.generated.h"

// @Owner: 이정준 (구현: Codex) | 2026-10-27
UINTERFACE(BlueprintType)
class PROJECT_NF_API UNF_UseItemInterface : public UInterface
{
	GENERATED_BODY()
};

class PROJECT_NF_API INF_UseItemInterface
{
	GENERATED_BODY()
public:
	// 소모품 컴포넌트에 호출한다. 슬롯 0~4. 클라이언트 호출은 서버에 요청만 한다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NF|Consumable|Use")
	void NF_UseItem(int32 SlotIndex = 0);
};
