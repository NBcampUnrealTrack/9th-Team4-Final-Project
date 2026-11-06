#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossPatternBase.h"
#include "NFGA_BossGroundDamageZone.generated.h"

class UGameplayEffect;
class UAnimMontage;

/**
 * Marks up to three random players, then creates short-lived damaging ground zones at their locations.
 * Flow: pattern start -> WarningMontage + warning decals (WarningDuration)
 *       -> zones turn on (ZoneStartMontage + GroundDamageZone.Start cue) -> damage ticks for ZoneDuration.
 */
UCLASS(Blueprintable)
class PROJECT_NF_API UNFGA_BossGroundDamageZone : public UNFGA_BossPatternBase
{
	GENERATED_BODY()

public:
	UNFGA_BossGroundDamageZone();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** Fired on the server for each zone when its damaging area becomes active. */
	UFUNCTION(BlueprintImplementableEvent, Category = "NF|Boss|Ground Damage Zone")
	void OnGroundDamageZoneStarted(FVector GroundLocation, float Radius);

	/** Fired on the server for each zone when its damaging area expires. */
	UFUNCTION(BlueprintImplementableEvent, Category = "NF|Boss|Ground Damage Zone")
	void OnGroundDamageZoneEnded(FVector GroundLocation, float Radius);

	/** Server event when the pattern starts (warning decals appear). Parent implementation plays WarningMontage. */
	UFUNCTION(BlueprintNativeEvent, Category = "NF|Boss|Ground Damage Zone|Presentation")
	void PlayWarningMontage();
	virtual void PlayWarningMontage_Implementation();

	/** Server event at the moment the zones turn on. Parent implementation plays ZoneStartMontage. */
	UFUNCTION(BlueprintNativeEvent, Category = "NF|Boss|Ground Damage Zone|Presentation")
	void PlayZoneStartMontage();
	virtual void PlayZoneStartMontage_Implementation();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern", meta = (ClampMin = "1.0", Units = "cm"))
	float TargetRange = 3000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern", meta = (ClampMin = "1", ClampMax = "3"))
	int32 MaxTargets = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern", meta = (ClampMin = "0.0", Units = "s"))
	float WarningDuration = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern", meta = (ClampMin = "1.0", Units = "cm"))
	float ZoneRadius = 350.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern", meta = (ClampMin = "0.0", Units = "s"))
	float ZoneDuration = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern", meta = (ClampMin = "0.01", Units = "s"))
	float DamageInterval = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Damage", meta = (ClampMin = "0.0"))
	float DamagePerTick = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Damage")
	FGameplayTag DamageMagnitudeTag;

	/** Warning decal shown on each zone until it turns on. Defaults to M_BossCircleTelegraph. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Telegraph",
		meta = (DisplayName = "Ground Damage Zone Telegraph Decal Material"))
	TObjectPtr<UMaterialInterface> TelegraphMaterial;

	/** Played once when the pattern starts, while the warning decals fill. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Animation")
	TObjectPtr<UAnimMontage> WarningMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Animation", meta = (ClampMin = "0.01"))
	float WarningMontagePlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Animation")
	FName WarningMontageStartSection;

	/** Played once at the moment the zones turn on (same frame as the first damage window opens). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Animation")
	TObjectPtr<UAnimMontage> ZoneStartMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Animation", meta = (ClampMin = "0.01"))
	float ZoneStartMontagePlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Animation")
	FName ZoneStartMontageStartSection;

private:
	void PlayBossMontage(UAnimMontage* Montage, float PlayRate, FName StartSection) const;
	void BeginDamageZones();
	void ApplyDamageTick();
	void EndDamageZones();
	void ExecuteZoneCue(const FGameplayTag CueTag, const FVector& Location) const;
	void ClearActiveZones();

	TArray<FVector> ZoneLocations;
	FTimerHandle WarningTimer;
	FTimerHandle DamageTickTimer;
	int32 DamageTickCount = 0;
	int32 MaxDamageTicks = 0;
	bool bZonesActive = false;
};
