#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "NF_GameMode.generated.h"

class ANF_GameState;
class APlayerController;

UCLASS()
class PROJECT_NF_API ANF_GameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ANF_GameMode();

	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

	// 서버에서 팀 배정과 PlayerState 반영을 완료한 뒤 호출
	void NotifyTeamsReady();

	// 보스 사망 처리와 열쇠 부여가 모두 성공한 뒤 호출
	void NotifyBossResolved();

	// 탈출 시스템이 서버에서 터치다운 조건을 검증한 뒤 호출
	void NotifyEscapeSucceeded();

private:
	static constexpr int32 RequiredPlayers = 8;

	UPROPERTY(EditDefaultsOnly, Category = "NF|Match")
	float BuildDuration = 60.0f;

	UPROPERTY(EditDefaultsOnly, Category = "NF|Match")
	float FarmingDuration = 900.0f;

	FTimerHandle PhaseTimerHandle;
	bool bTeamsReady = false;

	ANF_GameState* GetNFGameState() const;
	void TryStartBuild();
	void EnterBuild();
	void EnterFarming();
	void EnterBoss();
	void EnterEscape();
	void EnterFinished();
};