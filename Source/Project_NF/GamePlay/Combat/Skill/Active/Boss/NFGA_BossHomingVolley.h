#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossPatternBase.h"
#include "NFGA_BossHomingVolley.generated.h"

class ANF_BossHomingProjectile;
class UAnimMontage;
class UNiagaraSystem;

/** Select up to three nearby players and launch ground-born X/Z homing missiles. */
UCLASS(Blueprintable)
class PROJECT_NF_API UNFGA_BossHomingVolley : public UNFGA_BossPatternBase
{
	GENERATED_BODY()
public:
	UNFGA_BossHomingVolley();
	static constexpr float ShotInterval = 0.6f;
	static constexpr float RoundInterval = 5.f;
	static constexpr int32 RoundCount = 3;
protected:
	/** Runs once on the server for each successful shot. Use Spawn Replicated Boss Niagara in this event. */
	UFUNCTION(BlueprintImplementableEvent, Category = "NF|Boss|Homing|VFX")
	void OnHomingMissileLaunched(ANF_BossBase* Boss, ANF_BossHomingProjectile* Projectile, FTransform BossEffectTransform);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Homing|VFX")
	TObjectPtr<UNiagaraSystem> LaunchNiagaraSystem;
	/** None uses the boss mesh transform. Missing socket falls back to the mesh transform. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Homing|VFX")
	FName LaunchEffectSocket = NAME_None;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Homing|VFX")
	FVector LaunchEffectLocalOffset = FVector::ZeroVector;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Homing|VFX")
	FVector LaunchNiagaraScale = FVector::OneVector;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Homing", meta = (ClampMin = "1"))
	float TargetRange = 2500.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Homing", meta = (ClampMin = "1", ClampMax = "3"))
	int32 MaxTargets = 3;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Homing", meta = (ClampMin = "1"))
	float ProjectileSpeed = 600.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Homing", meta = (ClampMin = "1"))
	float DetectionRadius = 120.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Homing", meta = (ClampMin = "1"))
	float ExplosionRadius = 300.f;
	/** Spawn along world X toward the boss from each target; world Y stays on the target's launch plane. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Homing", meta = (ClampMin = "1"))
	float SpawnOffset = 600.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Homing", meta = (ClampMin = "0"))
	float AirborneLaunchVelocity = 650.f;
	/** Uses the existing DT_BossCombat RelocateDamage column for asset compatibility. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Homing", meta = (ClampMin = "0"))
	float Damage = 40.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Homing")
	TSubclassOf<ANF_BossHomingProjectile> ProjectileClass;
	/** Replayed for each successfully launched missile, independently of the GA's commit montage. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Homing|Animation")
	TObjectPtr<UAnimMontage> ShotMontage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Homing|Animation", meta = (ClampMin = "0.01"))
	float ShotMontagePlayRate = 1.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Homing|Animation")
	FName ShotMontageStartSection = NAME_None;
private:
	void BeginRound();
	void FireNextMissile();
	bool FireMissile(ANF_Player* Target);
	void FinishRound();
	void CheckProjectilesFinished();
	TArray<TWeakObjectPtr<ANF_Player>> RoundTargets;
	int32 StartedRounds = 0;
	int32 NextTargetIndex = 0;
	double LastShotTime = 0.;
	bool bAllRoundsFinished = false;
	TArray<TWeakObjectPtr<ANF_BossHomingProjectile>> Projectiles;
	FTimerHandle SequenceTimer;
	FTimerHandle CompletionTimer;
};
