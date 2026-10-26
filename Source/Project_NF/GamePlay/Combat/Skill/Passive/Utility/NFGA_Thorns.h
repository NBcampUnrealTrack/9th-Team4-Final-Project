#pragma once
#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/NFGA_SkillBase.h"
#include "NFGA_Thorns.generated.h"

// @Owner: 이정준 (구현: Codex) | 2026-10-22
// Observe actual health loss on the server; shield damage and overkill are excluded.
UCLASS(Blueprintable)
class PROJECT_NF_API UNFGA_Thorns : public UNFGA_SkillBase
{
	GENERATED_BODY()
public:
	UNFGA_Thorns();
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override { return false; }
	static float CalculateReflectedDamage(float OldHealth, float NewHealth, float Ratio);
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Thorns", meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "0.3 = 30% of actual health lost, excluding shield and overkill."))
	float ReflectionRatio = 0.3f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Thorns", meta = (ClampMin = "0.0", Units = "s"))
	float InternalCooldown = 1.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Thorns")
	TSubclassOf<UGameplayEffect> ReflectionEffectClass;
private:
	void BindHealth();
	void UnbindHealth();
	void HandleHealthChanged(const FOnAttributeChangeData& Data);
	TWeakObjectPtr<UAbilitySystemComponent> HealthASC;
	FDelegateHandle HealthDelegate;
	double NextReflectionTime = 0;
	bool bReflecting = false;
};
