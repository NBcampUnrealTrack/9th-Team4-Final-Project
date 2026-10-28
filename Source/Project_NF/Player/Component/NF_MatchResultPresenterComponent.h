#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/Struct/NF_MatchResult.h"
#include "NF_MatchResultPresenterComponent.generated.h"

class ANF_GameState;
class UNF_MatchResultWidget;

/** 로컬 PlayerController에서 결과 UI 표시와 매칭 복귀 요청을 처리한다. */
UCLASS(ClassGroup = (NF), meta = (BlueprintSpawnableComponent))
class PROJECT_NF_API UNF_MatchResultPresenterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNF_MatchResultPresenterComponent();

	// Controller의 자동 복귀 Client RPC에서 전달받은 결과를 사용한다.
	// GameState의 Finished 복제를 기다리지 않으며 세션 정리와 이동은 매칭 Subsystem에 위임한다.
	UFUNCTION(BlueprintCallable, Category = "NF|UI|MatchResult")
	void RequestReturnToMatchmaking(const FNF_MatchResult& Result);

	// 복귀 요청이 접수되기 전에 거절된 경우 버튼을 복구한다.
	UFUNCTION(BlueprintCallable, Category = "NF|UI|MatchResult")
	void NotifyReturnFailed();

	// 복귀 요청 접수 시 화면을 닫고 같은 결과의 재표시를 막는다.
	UFUNCTION(BlueprintCallable, Category = "NF|UI|MatchResult")
	void NotifyReturnCompleted();

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
	bool bEndingPlay = false;
};
