#pragma once
#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/NFGA_SkillBase.h"
#include "NFGA_WeaponPassive.generated.h"

// @Owner: 이정준 (구현: Codex) | 2026-10-21
UCLASS(Abstract)
class PROJECT_NF_API UNFGA_WeaponPassive : public UNFGA_SkillBase
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

UCLASS(Blueprintable)
class PROJECT_NF_API UNFGA_AttackPower : public UNFGA_WeaponPassive
{
	GENERATED_BODY()
public:
	UNFGA_AttackPower();
protected:
	virtual float GetStatMagnitude() const override { return AttackDamageBonus; }
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Attack Power", meta = (ClampMin = "0.0"))
	float AttackDamageBonus = 10.0f;
};

UCLASS(Blueprintable)
class PROJECT_NF_API UNFGA_MagicBullet : public UNFGA_WeaponPassive
{
	GENERATED_BODY()
public:
	UNFGA_MagicBullet();
protected:
	virtual float GetStatMagnitude() const override { return LastRoundDamageMultiplier; }
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Magic Bullet", meta = (ClampMin = "1.0"))
	float LastRoundDamageMultiplier = 2.0f;
};
