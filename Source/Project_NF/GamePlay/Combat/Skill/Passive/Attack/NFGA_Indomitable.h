#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/NFGA_SkillBase.h"
#include "NFGA_Indomitable.generated.h"

// @Owner: 이정준 (구현: Codex) | 2026-10-26
// 공통 즉시 피해 GE의 적용 전 검사. 플레이어 소스나 Health Setter를 사용하지 않는다.
struct FActiveGameplayEffectsContainer;
UCLASS(Blueprintable)
class PROJECT_NF_API UNFGA_Indomitable : public UNFGA_SkillBase
{
	GENERATED_BODY()
public:
	UNFGA_Indomitable();
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override { return false; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Indomitable", meta = (ClampMin = "0.0", Units = "s", ToolTip = "치명적인 공통 즉시 피해 GE를 차단한 뒤, 같은 피해 GE 경로를 계속 차단하는 시간입니다. 해당 GE의 쉴드 피해도 함께 차단합니다. 직접 체력 변경이나 이미 적용된 지속 피해는 대상이 아닙니다."))
	float ProtectionDuration = 3.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Indomitable", meta = (ClampMin = "0.0", Units = "s", ToolTip = "첫 치명 피해 GE의 적용을 거부한 시점부터 계산하는 서버 내부 재사용 대기시간입니다."))
	float CooldownDuration = 120.0f;
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "사용하지 않습니다. 불굴은 이제 체력 설정 대신 피해 GE 적용을 차단합니다."))
	float HealthBuffer = 1.0f;

private:
	friend class FNF_IndomitableTest;
	void BindDamageQuery();
	void QueueBindRetry(const TCHAR* Reason);
	void UnbindDamageQuery();
	bool AllowGameplayEffect(const FActiveGameplayEffectsContainer& Container, const FGameplayEffectSpec& Spec);
	bool PredictCommonDamage(const FGameplayEffectSpec& Spec, float& OutHealthDamage, float& OutShieldDamage) const;
	void EmitActivationCues();
	void EndProtection();
	TWeakObjectPtr<UAbilitySystemComponent> HealthASC;
	TWeakObjectPtr<AActor> ObservedAvatar;
	FDelegateHandle DamageQueryHandle;
	FTimerHandle ActivationCueTimer;
	float PendingTriggerDamage = 0.f;
	FTimerHandle ProtectionTimer;
	FTimerHandle BindRetryTimer;
	FString LastBindFailure;
	int32 BindRetryCount = 0;
	bool bRemoving = false;
	double ProtectionEndsAt = 0.0;
	double NextTriggerTime = 0.0;
	bool bHandlingDamage = false;
};
