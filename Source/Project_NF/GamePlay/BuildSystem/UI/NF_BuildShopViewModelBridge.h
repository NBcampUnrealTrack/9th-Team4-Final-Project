#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NF_BuildShopViewModelBridge.generated.h"

class UNF_BuildComponent;

// @Owner: 황준현 | 2026-10-29. ViewModel 담당자와 공유하는 C++ 연결 계약.
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UNF_BuildShopViewModelBridge : public UInterface
{
	GENERATED_BODY()
};

class PROJECT_NF_API INF_BuildShopViewModelBridge
{
	GENERATED_BODY()

public:
	// nullptr: 이전 소스 구독 해제와 표시 데이터 초기화.
	virtual void SetBuildSource(UNF_BuildComponent* InBuild) = 0;
	// Model을 통해 서버 상점 세션 종료 요청.
	virtual void RequestClose() = 0;
};
