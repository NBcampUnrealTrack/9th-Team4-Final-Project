#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossPatternBase.h"
#include "NFGA_BossLastGasp.generated.h"

class UGameplayEffect;
class ANF_Player;
class UMaterialInterface;
class UNiagaraSystem;

/** Death-only pattern: marks the ground under every living player in range with a warning decal,
 * then detonates those zones after TelegraphDuration, damaging and stunning every living player inside. */
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

	/** Server event, once per marked zone right after its warning decal is shown. */
	UFUNCTION(BlueprintImplementableEvent, Category = "NF|Boss|LastGasp|VFX")
	void OnLastGaspZoneMarked(FVector ZoneLocation, float ZoneRadius, float DetonationDelay);

	/** Server event, once per zone at detonation (before damage). Use Spawn Replicated Boss Niagara here
	 * for custom effects; ImpactNiagaraSystem (if set) is already spawned automatically. */
	UFUNCTION(BlueprintImplementableEvent, Category = "NF|Boss|LastGasp|VFX")
	void OnLastGaspZoneDetonated(FVector ZoneLocation, float ZoneRadius);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern", meta = (ClampMin = "0.0", Units = "s"))
	float TelegraphDuration = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern", meta = (ClampMin = "1.0", Units = "cm"))
	float ImpactRadius = 350.0f;

	/** Only living players within this distance of the boss are marked. 0 marks every living player. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern", meta = (ClampMin = "0.0", Units = "cm"))
	float TargetRange = 0.0f;

	/** Players this far above/below a zone's ground point are outside the blast. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern", meta = (ClampMin = "1.0", Units = "cm"))
	float ImpactHalfHeight = 300.0f;

	/** Circular decal material (Progress 0→1 over TelegraphDuration). Falls back to the boss telegraph material. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|LastGasp|VFX")
	TObjectPtr<UMaterialInterface> TelegraphMaterial;

	/** Optional one-shot Niagara replicated to clients at every zone on detonation. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|LastGasp|VFX")
	TObjectPtr<UNiagaraSystem> ImpactNiagaraSystem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|LastGasp|VFX")
	FVector ImpactNiagaraScale = FVector::OneVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|LastGasp|VFX", meta = (Units = "cm"))
	FVector ImpactNiagaraOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Damage")
	FGameplayTag DamageMagnitudeTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Damage")
	float Damage = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Damage")
	TSubclassOf<UGameplayEffect> StunEffectClass;

private:
	void DetonateMarkedZones();
	static bool IsLivingPlayer(const ANF_Player* Player);

	TArray<FVector> MarkedZones;
	FTimerHandle DetonationTimer;
};
