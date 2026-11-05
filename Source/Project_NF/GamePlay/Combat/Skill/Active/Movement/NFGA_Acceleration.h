#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/Active/NFGA_ActiveSkillBase.h"
#include "GamePlay/Combat/Skill/Execution/NF_AccelerationAfterimage.h"
#include "NFGA_Acceleration.generated.h"

class UMotionMatchingComponent;
class ANF_AccelerationEffect;

// @Owner: 이정준 (구현: Codex) | 2026-10-20
// 기존 플레이어 배율 API만 사용하는 시간제 가속화. 플레이어 코드는 수정하지 않는다.
UCLASS(Blueprintable)
class PROJECT_NF_API UNFGA_Acceleration : public UNFGA_ActiveSkillBase
{
	GENERATED_BODY()
public:
	UNFGA_Acceleration();
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Acceleration", meta = (ClampMin = "1.0", ToolTip = "1.3 = 30% faster. Applied after Cast Time for Execution Duration."))
	float SpeedMultiplier = 1.3f;
	// Material이 비어 있으면 잔상을 생성하지 않는다. 이동 효과에는 영향을 주지 않는다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Acceleration|Afterimage")
	FNF_AccelerationAfterimageSettings AfterimageSettings;
private:
	friend class FNF_AccelerationTest;
	bool HasValidSettings() const;
	void StartAcceleration();
	void CheckLifetime();
	UFUNCTION()
	void HandleAvatarDestroyed(AActor* DestroyedActor);
	TWeakObjectPtr<AActor> OriginalAvatar;
	TWeakObjectPtr<UMotionMatchingComponent> MovementComponent;
	UPROPERTY(Transient)
	TObjectPtr<ANF_AccelerationEffect> StandardMovementEffect;
	UPROPERTY(Transient)
	TObjectPtr<ANF_AccelerationAfterimage> AfterimageEffect;
	FTimerHandle CastTimer;
	FTimerHandle LifetimeTimer;
	double EndsAt = 0.0;
	float AppliedMultiplier = 1.0f;
	bool bMultiplierApplied = false;
};
