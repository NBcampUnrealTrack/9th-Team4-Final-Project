#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossPatternBase.h"
#include "NFGA_BossCenterBlast.generated.h"

class ANF_BossLineTelegraph;
class UAnimMontage;
class UGameplayEffect;
class UNiagaraSystem;

/**
 * Frontal strike combo. Repeats StrikeCount times:
 *   1) rectangular warning decal on the area in front of the boss (WarningDuration)
 *   2) strike montage (StrikeDelay from montage start to the hit)
 *   3) impact Niagara + damage/stun/knockback to players inside the exact warned rectangle, decal removed
 * then waits StrikeInterval before the next warning.
 */
UCLASS(Blueprintable)
class PROJECT_NF_API UNFGA_BossCenterBlast : public UNFGA_BossPatternBase
{
	GENERATED_BODY()

public:
	UNFGA_BossCenterBlast();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** Called each time a strike's warning decal is shown. */
	UFUNCTION(BlueprintImplementableEvent, Category = "NF|Boss|Pattern")
	void OnCenterBlastTelegraphStarted();

	/** Server event after a strike's warning ends. Parent implementation plays StrikeMontage on relevant clients. */
	UFUNCTION(BlueprintNativeEvent, Category = "NF|Boss|Front Strike|Presentation")
	void PlayStrikeMontage(int32 StrikeIndex);
	virtual void PlayStrikeMontage_Implementation(int32 StrikeIndex);

	/** Server event at the hit. Parent implementation replicates ImpactNiagaraSystem on the strike area.
	 * Cosmetic only: BP delays here do not delay damage. AreaTransform is the ground center + facing of the area. */
	UFUNCTION(BlueprintNativeEvent, Category = "NF|Boss|Front Strike|Presentation")
	void PlayStrikeNiagara(int32 StrikeIndex, FTransform AreaTransform);
	virtual void PlayStrikeNiagara_Implementation(int32 StrikeIndex, FTransform AreaTransform);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Front Strike", meta = (ClampMin = "1", ClampMax = "10"))
	int32 StrikeCount = 3;

	/** Distance from the boss's ground point to the near edge of the rectangle. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Front Strike", meta = (Units = "cm"))
	float StrikeForwardOffset = 0.0f;

	/** Rectangle length along the boss's forward direction. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Front Strike", meta = (ClampMin = "1.0", Units = "cm"))
	float StrikeLength = 1800.0f;

	/** Full rectangle width (left-right). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Front Strike", meta = (ClampMin = "1.0", Units = "cm"))
	float StrikeWidth = 1000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Front Strike", meta = (ClampMin = "1.0", Units = "cm"))
	float StrikeHalfHeight = 300.0f;

	/** How long each warning decal is shown before that strike's montage starts. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Front Strike", meta = (ClampMin = "0.01", Units = "s"))
	float WarningDuration = 1.5f;

	/** Game seconds from montage start to Niagara + damage; independent of montage play rate. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Front Strike", meta = (ClampMin = "0.0", Units = "s"))
	float StrikeDelay = 0.5f;

	/** Pause after a hit before the next warning appears. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Front Strike", meta = (ClampMin = "0.0", Units = "s"))
	float StrikeInterval = 0.5f;

	/** Pause after the last hit before the pattern ends. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Front Strike", meta = (ClampMin = "0.0", Units = "s"))
	float RecoveryDuration = 1.0f;

	/** Full-rectangle deferred decal material (no circular mask). Defaults to M_BossLineTelegraph. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Telegraph")
	TObjectPtr<UMaterialInterface> WarningMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Strike Animation")
	TObjectPtr<UAnimMontage> StrikeMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Strike Animation", meta = (ClampMin = "0.01"))
	float StrikeMontagePlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Strike Animation")
	FName StrikeMontageStartSection;

	/** Non-looping burst played on the strike area at the hit. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Impact VFX")
	TObjectPtr<UNiagaraSystem> ImpactNiagaraSystem;

	/** Offset in the area's local space (X = boss forward, Y = right, Z = up) from the area's ground center. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Impact VFX")
	FVector ImpactNiagaraOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Impact VFX")
	FRotator ImpactNiagaraLocalRotation = FRotator::ZeroRotator;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Impact VFX")
	FVector ImpactNiagaraScale = FVector::OneVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Damage")
	float Damage = 60.0f;

	/** Knockback away from the boss, applied on every hit. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Damage")
	float OutwardLaunchVelocity = 1100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Damage")
	float UpwardLaunchVelocity = 220.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Damage")
	FGameplayTag DamageMagnitudeTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Damage")
	TSubclassOf<UGameplayEffect> StunEffectClass;

private:
	float GetPlannedDuration() const;
	void BeginWarning();
	void BeginStrikeWindup();
	void Strike();
	void DestroyWarning();
	void FinishPattern();

	int32 CurrentStrikeIndex = 0;
	FTransform StrikeAreaTransform;
	FVector StrikeExtent = FVector::ZeroVector;
	UPROPERTY(Transient) TObjectPtr<ANF_BossLineTelegraph> Warning;
	FTimerHandle WarningTimer;
	FTimerHandle StrikeTimer;
	FTimerHandle NextStrikeTimer;
};
