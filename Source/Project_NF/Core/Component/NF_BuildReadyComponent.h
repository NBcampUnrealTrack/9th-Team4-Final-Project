#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "NF_BuildReadyComponent.generated.h"

class ANF_PlayerState;
class UAbilitySystemComponent;

// 기본 빌드 적용과 Ready 태그 갱신을 완료한 뒤 true를 반환
DECLARE_DELEGATE_RetVal_OneParam(bool,FNF_ApplyFallbackBuild,ANF_PlayerState*);

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

	// BuildSystem 연동 코드가 서버에서 바인딩
	FNF_ApplyFallbackBuild ApplyFallbackBuild;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	TMap<TWeakObjectPtr<ANF_PlayerState>, bool> PlayerBuildReadyStates;

	TMap<TWeakObjectPtr<ANF_PlayerState>,FNF_BuildReadyBinding> BuildReadyBindings;

	bool bResolvingBuilds = false;

	bool HasServerAuthority() const;

	void HandleBuildReadyTagChanged(const FGameplayTag Tag,int32 NewCount,TWeakObjectPtr<ANF_PlayerState> PlayerState);

	// 현재 서버 ASC의 태그를 기준으로 캐시 갱신
	void RefreshReadyStates();

	bool AreAllPlayersBuildReady(int32 RequiredPlayerCount) const;

	void ClearBindings();
};