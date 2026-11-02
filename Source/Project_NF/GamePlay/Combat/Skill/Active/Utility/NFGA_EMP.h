#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/Active/NFGA_ActiveSkillBase.h"
#include "GamePlay/Combat/Components/NF_TargetQueryTypes.h"
#include "NFGA_EMP.generated.h"

class UNFGE_EMP;

/** Instant radial pulse. The same GE can also be applied by HitEffect / ApplyEffect compositions. */
UCLASS(Blueprintable)
class PROJECT_NF_API UNFGA_EMP : public UNFGA_ActiveSkillBase
{
	GENERATED_BODY()
public:
	UNFGA_EMP();
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|EMP", meta = (ClampMin = "1", Units = "cm"))
	float Radius = 600.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|EMP", meta = (ClampMin = "0.05", Units = "s"))
	float SealDuration = 3.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|EMP")
	TSubclassOf<UNFGE_EMP> EMPEffectClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|EMP")
	FNF_TargetFilterSettings TargetFilter;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|EMP")
	bool bRequireLineOfSight = true;
private:
	bool HasValidSettings() const;
	void EmitPulse();
	FTimerHandle CastTimer;
	TWeakObjectPtr<AActor> OriginalAvatar;
};
