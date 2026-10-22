#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "NF_BossContributionRewardComponent.generated.h"

class ANF_PlayerState;
class UCurveFloat;
class UGameplayEffect;
class UAbilitySystemComponent;

struct FNF_BossDefeatResult;

UCLASS(ClassGroup = (NF))
class PROJECT_NF_API UNF_BossContributionRewardComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNF_BossContributionRewardComponent();
	
	// 확정된 보스 처치 기여도 기준 플레이어별 회복 보상 적용
	bool ApplyRewards(const FNF_BossDefeatResult& DefeatResult);

	// 처치 시점의 접속 참가자와 팀별 피해를 고정한다. 회복 보상과 독립적으로 처리한다.
	bool PrepareTeamPenalty(const FNF_BossDefeatResult& DefeatResult);
	bool ApplyTeamPenalty();
	void CleanupTeamPenalty();

	bool IsRewardApplied() const
	{
		return bRewardApplied;
	}

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	struct FTeamPenaltyMember
	{
		TWeakObjectPtr<ANF_PlayerState> PlayerState;
		int32 TeamId = INDEX_NONE;
	};

	struct FAppliedTeamPenalty
	{
		TWeakObjectPtr<UAbilitySystemComponent> ASC;
		FActiveGameplayEffectHandle Handle;
	};

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

	// 지우 파트에서 제공하는 최대 실드량 감소 Infinite GE. 수치 계산은 GE에서 담당한다.
	UPROPERTY(EditDefaultsOnly, Category = "NF|Boss|Penalty")
	TSubclassOf<UGameplayEffect> LowestTeamPenaltyEffectClass;

	TArray<FTeamPenaltyMember> PenaltyMembers;
	TSet<int32> LowestContributionTeams;
	TSet<TWeakObjectPtr<ANF_PlayerState>> ResolvedPenaltyPlayers;
	TArray<FAppliedTeamPenalty> AppliedTeamPenalties;

	bool bTeamPenaltyPrepared = false;
	bool bApplyingTeamPenalty = false;
	bool bTeamPenaltyCleaned = false;
};
