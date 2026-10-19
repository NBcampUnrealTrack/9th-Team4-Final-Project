#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/Struct/NF_BossDefeatResult.h"
#include "NF_BossResolutionComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FNF_OnBossDefeatResolved,const FNF_BossDefeatResult&);


 //서버에서 보스 처치 결과를 검증하고 한 번만 확정
UCLASS(ClassGroup = (NF))
class PROJECT_NF_API UNF_BossResolutionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNF_BossResolutionComponent();

	//보스 시스템이 전달한 처치 결과를 검증하고 확정
	bool ResolveBossDefeat(const FNF_BossDefeatResult& DefeatResult);

	bool IsBossDefeatResolved() const
	{
		return bBossDefeatResolved;
	}

	const FNF_BossDefeatResult& GetConfirmedResult() const
	{
		return ConfirmedResult;
	}

	FNF_OnBossDefeatResolved OnBossDefeatResolved;

private:
	bool ValidateDefeatResult(const FNF_BossDefeatResult& DefeatResult) const;

	FNF_BossDefeatResult MakeConfirmedResult(const FNF_BossDefeatResult& DefeatResult) const;

	bool bBossDefeatResolved = false;

	UPROPERTY()
	FNF_BossDefeatResult ConfirmedResult;
};