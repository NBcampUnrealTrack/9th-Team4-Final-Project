#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossPatternBase.h"
#include "NFGA_BossAreaVolley.generated.h"

class ANF_BossAreaProjectile;
class UGameplayEffect;
class UAnimMontage;
class UNiagaraSystem;
class UMaterialInterface;

/** Server-authoritative boss pattern: target snapshot, telegraph, then fixed-point volleys. */
UCLASS(Blueprintable)
class PROJECT_NF_API UNFGA_BossAreaVolley : public UNFGA_BossPatternBase
{
	GENERATED_BODY()

public:
	UNFGA_BossAreaVolley();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

	/** Called on the server once when a volley has at least one valid telegraphed target.
	 *  Blueprint children can start Play Montage and Wait here for the volley wind-up animation.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "NF|Boss|Pattern")
	void OnVolleyTelegraphStarted(int32 TargetCount);

	/** Called once per volley immediately before its first projectile is spawned. */
	UFUNCTION(BlueprintImplementableEvent, Category = "NF|Boss|Pattern")
	void OnVolleyProjectileLaunch();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern", meta = (ClampMin = "1.0"))
	float PatternDuration = 60.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern", meta = (ClampMin = "0.1"))
	float VolleyInterval = 2.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern", meta = (ClampMin = "1"))
	int32 MaxVolleysPerPattern = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern", meta = (ClampMin = "0.0"))
	float TelegraphDuration = 2.0f;

	/** Warning decal shown at each landing point until the projectile hits. Empty uses the boss's TelegraphDecalMaterial. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Telegraph",
		meta = (DisplayName = "Area Volley Telegraph Decal Material"))
	TObjectPtr<UMaterialInterface> TelegraphMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern", meta = (ClampMin = "1.0", Units = "cm"))
	float TargetRange = 3000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern", meta = (ClampMin = "1", ClampMax = "5"))
	int32 MaxTargetsPerVolley = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern", meta = (ClampMin = "1.0", Units = "cm"))
	float ImpactRadius = 350.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern", meta = (ClampMin = "1.0", Units = "cm/s"))
	float ProjectileSpeed = 2500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern", meta = (ClampMin = "1.0", Units = "cm"))
	float GroundTraceDistance = 5000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Damage")
	FGameplayTag DamageMagnitudeTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Damage", meta = (ClampMin = "0.0"))
	float Damage = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Projectile")
	TSubclassOf<ANF_BossAreaProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Projectile")
	FName ProjectileSpawnSocketName = TEXT("FirePoint");

	/** Played once per simultaneous volley, independently of the ability's lifetime. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Launch Animation")
	TObjectPtr<UAnimMontage> ShotMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Launch Animation", meta = (ClampMin = "0.01"))
	float ShotMontagePlayRate = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Launch Animation")
	FName ShotMontageStartSection;

	/** Seconds after montage start before muzzle flashes and all projectiles launch together.
	 * Measured in game seconds, not montage time; adjust when changing montage play rate.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Launch Animation", meta = (ClampMin = "0.0", Units = "s"))
	float ProjectileLaunchDelay = 0.f;

	/** Non-looping muzzle flash, played at both socket transforms at launch time. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Launch VFX")
	TObjectPtr<UNiagaraSystem> LaunchNiagaraSystem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Launch VFX")
	FName LeftMuzzleSocket;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Launch VFX")
	FName RightMuzzleSocket;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Launch VFX")
	FVector LaunchEffectLocalOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Launch VFX")
	FRotator LaunchEffectLocalRotation = FRotator::ZeroRotator;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Launch VFX")
	FVector LaunchNiagaraScale = FVector::OneVector;

private:
	void PlayVolleyLaunchCosmetics();
	void BeginVolleyWindup(TArray<FVector> Targets);
	void LaunchVolley(TArray<FVector> Targets);
	void StartVolley();
	void FinishTelegraph(FVector_NetQuantize TargetGroundPoint);
	bool FindGroundPoint(AActor* Target, FVector& OutGroundPoint) const;
	void EndPattern();

	FTimerHandle VolleyTimer;
	FTimerHandle EndTimer;
	TArray<FTimerHandle> TelegraphTimers;
	float ElapsedPatternTime = 0.0f;
	int32 StartedVolleyCount = 0;
	bool bWarnedMissingDamageConfig = false;

};
