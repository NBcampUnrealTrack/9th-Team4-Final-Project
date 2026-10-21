#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "NF_BossContributionRewardComponent.generated.h"

class ANF_PlayerState;
class UCurveFloat;
class UGameplayEffect;

struct FNF_BossDefeatResult;

UCLASS(ClassGroup = (NF))
class PROJECT_NF_API UNF_BossContributionRewardComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNF_BossContributionRewardComponent();
	
	// 확정된 보스 처치 기여도 기준 플레이어별 회복 보상 적용
	bool ApplyRewards(const FNF_BossDefeatResult& DefeatResult);

	bool IsRewardApplied() const
	{
		return bRewardApplied;
	}

private:
	bool HasServerAuthority() const;

	bool BuildDamageByPlayer(
		const FNF_BossDefeatResult& DefeatResult,
		TMap<ANF_PlayerState*, float>& OutDamageByPlayer,
		float& OutTotalDamage) const;

	bool ApplyRecoveryEffect(ANF_PlayerState* PlayerState,float RecoveryAmount);
	
	UPROPERTY(EditDefaultsOnly,Category = "NF|Boss|Reward")
	TSubclassOf<UGameplayEffect>ContributionRecoveryEffectClass;
	
	//지우님과 합의할 SetByCaller 태그.
	//예: Data.Reward.BossContributionRecovery
	UPROPERTY(EditDefaultsOnly,Category = "NF|Boss|Reward")
	FGameplayTag RecoveryMagnitudeTag;
	
	//X: 기여도(비율 0~1) 대비 최대 HP 기준 회복(비율 0~1) 
	UPROPERTY(EditDefaultsOnly,Category = "NF|Boss|Reward")
	TObjectPtr<UCurveFloat> ContributionRecoveryCurve;

	bool bRewardApplied = false;
};