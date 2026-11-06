#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/Active/NFGA_ActiveSkillBase.h"
#include "NFGA_BossPatternBase.generated.h"

class ANF_BossBase;
class ANF_BossAreaProjectile;
class UGameplayEffect;
class UMaterialInterface;
class UNF_BossGEDataAsset;

/** Shared server-authoritative GAS helpers for boss pattern abilities. */
UCLASS(Abstract, Blueprintable)
class PROJECT_NF_API UNFGA_BossPatternBase : public UNFGA_ActiveSkillBase
{
	GENERATED_BODY()

public:
	UNFGA_BossPatternBase();
	bool WasLastExecutionCancelled() const { return bLastExecutionCancelled; }

protected:
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	ANF_BossBase* GetBoss() const;
	/** Returns Override when set; otherwise the GE registered in DA_NF_BossGE through the GE Manager.
	 * Logs an error when neither is available. Call at runtime only (the GE Manager needs a GameInstance). */
	TSubclassOf<UGameplayEffect> ResolveBossEffect(TSubclassOf<UGameplayEffect> Override,
		TSubclassOf<UGameplayEffect> UNF_BossGEDataAsset::* Member, const TCHAR* DebugName) const;
	FGameplayEffectSpecHandle MakeBossEffectSpec(TSubclassOf<UGameplayEffect> EffectClass,
		FGameplayTag MagnitudeTag, float Magnitude) const;
	void ApplyEffectToActor(AActor* Target, const FGameplayEffectSpecHandle& Spec) const;
	void ApplyDamageAndStun(AActor* Target, TSubclassOf<UGameplayEffect> DamageEffect,
		FGameplayTag DamageMagnitudeTag, float Damage, TSubclassOf<UGameplayEffect> StunEffect) const;
	void ShowTelegraph(const FVector& Location, float Radius, float Duration, bool bHeadWarning = false,
		float DecalLifetimeOverride = 0.0f, UMaterialInterface* MaterialOverride = nullptr) const;
	bool FindGroundPoint(AActor* Target, FVector& OutGroundPoint, float TraceDistance = 5000.0f) const;
	TArray<class ANF_Player*> FindPlayersInRange(const FVector& Center, float Radius) const;
	ANF_BossAreaProjectile* SpawnAreaProjectile(TSubclassOf<ANF_BossAreaProjectile> ProjectileClass,
		const FVector& TargetGroundPoint, float Radius, float Speed,
		const FGameplayEffectSpecHandle& DamageSpec, FGameplayTag ImpactCueTag) const;
	void StartAbilityTimer(FTimerHandle& TimerHandle, float Delay, TFunction<void()> Callback);

private:
	bool bLastExecutionCancelled = false;
};
