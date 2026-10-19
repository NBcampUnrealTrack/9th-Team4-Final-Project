#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossPatternBase.h"
#include "NFGA_BossGroundDamageZone.generated.h"

class UGameplayEffect;

/** Marks up to three random players, then creates short-lived damaging ground zones at their locations. */
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

private:
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
