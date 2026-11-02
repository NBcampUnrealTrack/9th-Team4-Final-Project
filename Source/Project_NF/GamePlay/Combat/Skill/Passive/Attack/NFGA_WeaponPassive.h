#pragma once
#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/Passive/NFGA_PassiveSkillBase.h"
#include "NFGA_WeaponPassive.generated.h"

// @Owner: 이정준 (구현: Codex) | 2026-10-21
// Shared equip/unequip handling only. Concrete skills have their own files.
UCLASS(Abstract)
class PROJECT_NF_API UNFGA_WeaponPassive : public UNFGA_PassiveSkillBase
{
	GENERATED_BODY()
public:
	UNFGA_WeaponPassive();
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override { return false; }
protected:
	virtual float GetStatMagnitude() const { return 0.0f; }
	UPROPERTY()
	TSubclassOf<UGameplayEffect> StatEffectClass;
	FGameplayTag MagnitudeTag;
private:
	void PrepareOffenseAttributes(const FGameplayAbilityActorInfo* ActorInfo);
	void RefreshStatEffect();
	void RemoveStatEffect();
	TWeakObjectPtr<UAbilitySystemComponent> StatASC;
	FActiveGameplayEffectHandle StatEffect;
};
