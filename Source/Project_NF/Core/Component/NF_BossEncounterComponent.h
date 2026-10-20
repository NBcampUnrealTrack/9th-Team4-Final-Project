#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/NF_CoreIntegrationTypes.h"
#include "NF_BossEncounterComponent.generated.h"

class ANF_BossBase;

DECLARE_MULTICAST_DELEGATE_OneParam(FNF_OnBossSpawnSucceeded,ANF_BossBase*);

DECLARE_MULTICAST_DELEGATE_OneParam(FNF_OnBossSpawnFailed,const FString&);


//GameMode에 부착하는 서버 전용 컴포넌트. 보스 출현 요청과 결과 검증을 담당
UCLASS()
class PROJECT_NF_API UNF_BossEncounterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNF_BossEncounterComponent();

	void RequestSpawn();
	void CancelEncounter();
	
	bool CanRegisterSpawnHandler() const;

	ANF_BossBase* GetActiveBoss() const;

	FNF_RequestBossSpawn RequestBossSpawn;

	FNF_OnBossSpawnSucceeded OnBossSpawnSucceeded;
	FNF_OnBossSpawnFailed OnBossSpawnFailed;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
private:
	TWeakObjectPtr<ANF_BossBase> ActiveBoss;

	// 이 컴포넌트는 매치당 한 번의 보스 출현을 처리한다.
	bool bSpawnRequested = false;
	bool bCancelled = false;

	bool HasServerAuthority() const;

	void FailSpawn(const FString& Reason);
};