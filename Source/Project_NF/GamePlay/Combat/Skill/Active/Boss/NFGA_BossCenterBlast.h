#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossPatternBase.h"
#include "NFGA_BossCenterBlast.generated.h"

class UGameplayEffect;

/** Five-second boss-centered telegraph followed by damage, outward knockback, and optional stun GE. */
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

	/** Called immediately after the center blast telegraph is shown. */
	UFUNCTION(BlueprintImplementableEvent, Category = "NF|Boss|Pattern")
	void OnCenterBlastTelegraphStarted();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern")
	float TelegraphDuration = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern")
	float BlastRadius = 1800.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern")
	float Damage = 60.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern")
	float OutwardLaunchVelocity = 1100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern")
	float UpwardLaunchVelocity = 220.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Damage")
	FGameplayTag DamageMagnitudeTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Damage")
	TSubclassOf<UGameplayEffect> StunEffectClass;

private:
	void Detonate();
	FTimerHandle DetonationTimer;
};
