#pragma once
#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/NFGA_SkillBase.h"
#include "NFGA_Berserker.generated.h"

// @Owner: 이정준 (구현: Codex) | 2026-10-22
// Server-side stacks from actual Health damage. Owns only its attack bonus GE.
UCLASS(Blueprintable)
class PROJECT_NF_API UNFGA_Berserker : public UNFGA_SkillBase
{
	GENERATED_BODY()
public:
	UNFGA_Berserker();
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override { return false; }
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Berserker", meta = (ClampMin = "0.0"))
	float DamageBonusPerStack = 3.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Berserker", meta = (ClampMin = "1"))
	int32 MaxStacks = 5;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Berserker", meta = (ClampMin = "0.0", Units = "s", ToolTip = "0: until death/unequip. Otherwise each health hit refreshes all stacks; they expire together."))
	float StackDuration = 0.0f;
private:
	friend class FNF_BerserkerTest;
	void BindHealth();
	void UnbindHealth();
	void ResetStacks();
	void HandleHealthChanged(const FOnAttributeChangeData& Data);
	void HandleDeathTag(FGameplayTag Tag, int32 Count);
	bool IsDead() const;
	TWeakObjectPtr<UAbilitySystemComponent> HealthASC;
	TWeakObjectPtr<UAbilitySystemComponent> BonusASC;
	TWeakObjectPtr<AActor> ObservedAvatar;
	FDelegateHandle HealthDelegate;
	FDelegateHandle HealthDeathDelegate;
	FDelegateHandle OwnDeathDelegate;
	FActiveGameplayEffectHandle BonusEffect;
	FTimerHandle ExpiryTimer;
	int32 StackCount = 0;
};
