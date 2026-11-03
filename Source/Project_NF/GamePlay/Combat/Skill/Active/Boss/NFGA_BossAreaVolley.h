#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossPatternBase.h"
#include "NFGA_BossAreaVolley.generated.h"

class ANF_BossAreaProjectile;
class UGameplayEffect;

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
	float VolleyInterval = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern", meta = (ClampMin = "1"))
	int32 MaxVolleysPerPattern = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern", meta = (ClampMin = "0.0"))
	float TelegraphDuration = 2.0f;

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

private:
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
	bool bLaunchEventTriggeredForVolley = false;
};
