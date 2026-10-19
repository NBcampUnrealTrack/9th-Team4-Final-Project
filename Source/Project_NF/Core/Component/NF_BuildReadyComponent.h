#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Core/NF_CoreIntegrationTypes.h"
#include "NF_BuildReadyComponent.generated.h"

class ANF_PlayerState;
class UAbilitySystemComponent;


DECLARE_MULTICAST_DELEGATE(FNF_OnBuildResolveSucceeded);
DECLARE_MULTICAST_DELEGATE_OneParam(FNF_OnBuildResolveFailed,const FString&);

struct FNF_BuildReadyBinding
{
	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystem;
	FDelegateHandle DelegateHandle;
};

/**
 * @owner : 민웅 | 2026-10-16
 * GameMode에 부착하는 서버 전용 컴포넌트. 플레이어의 빌드 준비 상태와 기본 빌드 적용 요청을 관리.
 */
UCLASS()
class PROJECT_NF_API UNF_BuildReadyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNF_BuildReadyComponent();

	void RegisterPlayer(ANF_PlayerState* PlayerState);
	void UnregisterPlayer(ANF_PlayerState* PlayerState);
	
	 // 미준비 플레이어에게 기본 빌드를 요청, 최종 준비 상태를 확인. 모두 준비된 경우에만 true 반환
	bool ResolveUnreadyPlayers(int32 RequiredPlayerCount);

	// Build 제한시간 종료 시 GameMode가 호출
	void BeginResolve();

	// 매치 중단·종료 또는 컴포넌트 종료 시 호출
	void CancelResolve();

	FNF_OnBuildResolveSucceeded OnBuildResolveSucceeded;
	FNF_OnBuildResolveFailed OnBuildResolveFailed;
	
	// BuildSystem 연동 코드가 서버에서 바인딩
	FNF_ApplyFallbackBuild ApplyFallbackBuild;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	
	UPROPERTY(EditDefaultsOnly, Category = "NF|Build|Deadline",meta = (ClampMin = "1"))
	int32 MaxResolveAttempts = 3;

	UPROPERTY(EditDefaultsOnly, Category = "NF|Build|Deadline",meta = (ClampMin = "0.1"))
	float ResolveRetryInterval = 1.0f;
	
	int32 ResolveAttempts = 0;
	
	bool bDeadlineResolving = false;
	
	bool bResolvingBuilds = false;
	
	TMap<TWeakObjectPtr<ANF_PlayerState>, bool> PlayerBuildReadyStates;
	
	TMap<TWeakObjectPtr<ANF_PlayerState>,FNF_BuildReadyBinding> BuildReadyBindings;

	FTimerHandle ResolveRetryTimerHandle;
	
	
	bool HasServerAuthority() const;

	void HandleBuildReadyTagChanged(const FGameplayTag Tag,int32 NewCount,TWeakObjectPtr<ANF_PlayerState> PlayerState);

	// 현재 서버 ASC의 태그를 기준으로 캐시 갱신
	void RefreshReadyStates();

	bool AreAllPlayersBuildReady(int32 RequiredPlayerCount) const;

	void ClearBindings();
	
	void TryResolve();
	
	void FinishResolveFailure(const FString& Reason);
	
};