#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/Active/NFGA_ActiveSkillBase.h"
#include "GamePlay/Combat/Components/NF_TargetQueryTypes.h"
#include "NFGA_Clairvoyance.generated.h"

class ANF_PlayerController;
class UNF_MinimapRevealRegistry;

// @owner: 이정준 (구현: Codex) | 2026-10-15
// 목적: 서버 범위 탐색을 공통 공개 목록에 연결하는 천리안 전용 GA. 추가 컴포넌트 없음.
UCLASS(Blueprintable)
class PROJECT_NF_API UNFGA_Clairvoyance : public UNFGA_ActiveSkillBase
{
	GENERATED_BODY()
public:
	UNFGA_Clairvoyance();
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
protected:


	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Clairvoyance", meta = (ClampMin = "1.0", Units = "cm"))
	float DetectionRadius = 2000.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Clairvoyance", meta = (ClampMin = "0.05", Units = "s"))
	float DetectionInterval = 0.2f;
	// 현재 팀 정보가 없어 All로 시작한다. 사망/기절 제외 태그는 팀 정책에 맞춰 지정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Clairvoyance")
	FNF_TargetFilterSettings TargetFilter;
private:
	bool HasValidClairvoyanceSettings() const;
	void StartDetection();
	void ScanTargets();
	void FinishDetection();
	TWeakObjectPtr<ANF_PlayerController> Recipient;
	TWeakObjectPtr<UNF_MinimapRevealRegistry> Registry;
	TWeakObjectPtr<AActor> ActivationAvatar;
	FGuid RevealSourceId;
	FTimerHandle CastTimer;
	FTimerHandle ScanTimer;
	FTimerHandle EndTimer;
	double EndsAt = 0.0;
};
