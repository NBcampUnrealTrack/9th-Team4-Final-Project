#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/Struct/NF_MatchResult.h"
#include "NF_MatchResultPresenterComponent.generated.h"

class ANF_GameState;
class UNF_MatchResultWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNF_OnPresenterReturnToMatchmakingRequested);

/** 로컬 PlayerController에서 서버의 매치 결과를 결과 UI에 연결한다. */
UCLASS(ClassGroup = (NF), meta = (BlueprintSpawnableComponent))
class PROJECT_NF_API UNF_MatchResultPresenterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNF_MatchResultPresenterComponent();

	// 요청 거절 또는 비동기 실패 시 버튼을 복구한다.
	UFUNCTION(BlueprintCallable, Category = "NF|UI|MatchResult")
	void NotifyReturnFailed();

	// 실제 복귀 완료 시 화면을 닫고 같은 결과의 재표시를 막는다.
	UFUNCTION(BlueprintCallable, Category = "NF|UI|MatchResult")
	void NotifyReturnCompleted();

	// 실제 서버 이탈과 세션 정리는 매칭 담당 코드에서 연결한다.
	UPROPERTY(BlueprintAssignable, Category = "NF|UI|MatchResult")
	FNF_OnPresenterReturnToMatchmakingRequested OnReturnToMatchmakingRequested;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditDefaultsOnly, Category = "NF|UI|MatchResult")
	TSubclassOf<UNF_MatchResultWidget> MatchResultWidgetClass;

private:
	void RefreshResultScreen();
	void ReleaseResultWidget();
	bool HasResultChanged(const FNF_MatchResult& Result, int32 LocalTeamId) const;

	UFUNCTION()
	void HandleReturnRequested();

	UPROPERTY(Transient)
	TObjectPtr<UNF_MatchResultWidget> ResultWidget;

	TWeakObjectPtr<ANF_GameState> ObservedGameState;
	FNF_MatchResult LastPresentedResult;
	int32 LastPresentedTeamId = INDEX_NONE;
	bool bHasPresentedResult = false;
	bool bReturnPending = false;
	bool bPresentationCompleted = false;
};
