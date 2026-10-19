#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NF_BuildMatchBridgeComponent.generated.h"

class ANF_PlayerState;
class UNF_BuildReadyComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNF_OnFallbackBuildCompleted,
	ANF_PlayerState*, PlayerState, bool, bSucceeded);

/** @Owner: 황준현 | 2026-10-19. GameMode에 하나만 추가하는 서버 빌드 연동 컴포넌트. */
UCLASS(ClassGroup = (NF), meta = (BlueprintSpawnableComponent))
class PROJECT_NF_API UNF_BuildMatchBridgeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNF_BuildMatchBridgeComponent();

	/** 서버의 결과 알림이다. 네트워크 멀티캐스트 RPC가 아니다. */
	UPROPERTY(BlueprintAssignable, Category = "NF|Build|Fallback")
	FNF_OnFallbackBuildCompleted OnFallbackBuildCompleted;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool HandleApplyFallbackBuild(ANF_PlayerState* PlayerState);
	TWeakObjectPtr<UNF_BuildReadyComponent> BoundReadyComponent;
};
