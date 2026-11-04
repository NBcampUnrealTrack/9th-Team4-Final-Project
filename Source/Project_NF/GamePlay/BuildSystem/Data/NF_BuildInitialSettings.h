#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NF_BuildInitialSettings.generated.h"

/** @Owner: 황준현 | 2026-11-04. 공통 시작 설정. 런타임 플레이어 상태는 컴포넌트가 보관한다. */
UCLASS(BlueprintType)
class PROJECT_NF_API UNF_BuildInitialSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Build", meta = (ClampMin = "0"))
	int32 InitialCostLimit = 10;
};
