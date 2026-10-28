#pragma once

#include "CoreMinimal.h"
#include "Core/NF_GameMode.h"
#include "NF_TestGameMode.generated.h"

// Steam 매치메이킹 없이 직접 접속하여 인게임 흐름을 확인하는 테스트 전용 게임모드.
UCLASS()
class PROJECT_NF_API ANF_TestGameMode : public ANF_GameMode
{
	GENERATED_BODY()

public:
	ANF_TestGameMode();

	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual bool ShouldUseDedicatedMatchSubsystem() const override;

private:
	TArray<TWeakObjectPtr<ANF_PlayerState>> LoginOrder;
	FTimerHandle TeamAssignmentTimerHandle;
	bool bTeamAssignmentStopped = false;

	void TryAssignTeams();
	void StopTeamAssignment();
};
