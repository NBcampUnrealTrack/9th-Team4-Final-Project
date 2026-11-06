#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossPatternBase.h"
#include "NFGA_BossThreatVolley.generated.h"

class ANF_BossAreaProjectile;
class ANF_Player;
class UGameplayEffect;
class UMaterialInterface;

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

	/** Warning decal shown at each shot's landing point. It fills and disappears exactly when that shot hits
	 * (damage + impact Niagara), so its lifetime is the shot's flight time. Empty uses the boss's TelegraphDecalMaterial. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Telegraph",
		meta = (DisplayName = "Threat Volley Telegraph Decal Material"))
	TObjectPtr<UMaterialInterface> ShotTelegraphMaterial = nullptr;

	/** The boss turns toward the selected target continuously for the whole pattern at this yaw speed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern", meta = (ClampMin = "0.0", Units = "deg"))
	float TurnRateDegreesPerSecond = 45.0f;

	/** After the last shot the boss turns back to its pre-pattern yaw at this speed. The pattern (recovery)
	 * lasts until the turn back completes, so a large angle extends recovery beyond RecoveryDuration. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern", meta = (ClampMin = "1.0", Units = "deg"))
	float ReturnTurnRateDegreesPerSecond = 90.0f;

	/** How often the facing is updated. Small values look smooth; 0.016 ≈ every frame at 60 FPS. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern", meta = (ClampMin = "0.005", Units = "s"))
	float FacingUpdateInterval = 0.016f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern")
	float ImpactRadius = 220.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern")
	float ProjectileSpeed = 2000.0f;

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
	void UpdateFacing();

	TWeakObjectPtr<ANF_Player> SelectedTarget;
	FTimerHandle FacingTimer;
	double LastFacingUpdateTime = 0.0;
	float PatternStartYaw = 0.0f;
	bool bReturningToStartYaw = false;
	int32 FiredProjectileCount = 0;
	FTimerHandle ChargeTimer;
	FTimerHandle TelegraphTimer;
	FTimerHandle ShotTimer;
	FTimerHandle RecoveryTimer;
	float ElapsedFireTime = 0.0f;
};
