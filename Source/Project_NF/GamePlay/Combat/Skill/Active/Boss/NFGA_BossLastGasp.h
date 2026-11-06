#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossPatternBase.h"
#include "NFGA_BossLastGasp.generated.h"

class UGameplayEffect;
class ANF_Player;

/** Death-only pattern: marks every player's current position, then detonates those zones after four seconds. */
UCLASS(Blueprintable)
class PROJECT_NF_API UNFGA_BossLastGasp : public UNFGA_BossPatternBase
{
	GENERATED_BODY()

public:
	UNFGA_BossLastGasp();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern")
	float TelegraphDuration = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern")
	float ImpactRadius = 350.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Damage")
	FGameplayTag DamageMagnitudeTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Damage")
	float Damage = 100.0f;

	/** Optional override. Empty uses BossStunEffectClass from DA_NF_BossGE (GE Manager). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Damage")
	TSubclassOf<UGameplayEffect> StunEffectClass;

private:
	void DetonateMarkedZones();

	TMap<TWeakObjectPtr<ANF_Player>, FVector> MarkedLocations;
	FTimerHandle DetonationTimer;
};
