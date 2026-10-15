#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossPatternBase.h"
#include "NFGA_BossThreatVolley.generated.h"

class ANF_BossAreaProjectile;
class ANF_Player;
class UGameplayEffect;

/** Selects a random player in range, warns them, then fires a rotating aimed volley. */
UCLASS(Blueprintable)
class PROJECT_NF_API UNFGA_BossThreatVolley : public UNFGA_BossPatternBase
{
	GENERATED_BODY()

public:
	UNFGA_BossThreatVolley();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern")
	float TargetRange = 3000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern")
	float ChargeDuration = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern")
	float TelegraphDuration = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern", meta = (ClampMin = "0.0", Units = "s"))
	float ProjectileFireDuration = 6.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern")
	float ShotInterval = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern")
	float TurnRateDegreesPerSecond = 45.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern")
	float ImpactRadius = 220.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern")
	float ProjectileSpeed = 2500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Recovery")
	float RecoveryDuration = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Projectile")
	TSubclassOf<ANF_BossAreaProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Damage")
	FGameplayTag DamageMagnitudeTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Damage")
	float Damage = 25.0f;

private:
	void BeginAimedVolley();
	void FireNextProjectile();
	void FinishPattern();

	TWeakObjectPtr<ANF_Player> SelectedTarget;
	FTimerHandle ChargeTimer;
	FTimerHandle TelegraphTimer;
	FTimerHandle ShotTimer;
	FTimerHandle RecoveryTimer;
	float ElapsedFireTime = 0.0f;
};
