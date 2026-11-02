#pragma once
#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "NF_TmpSkillBase.generated.h"

class UNF_TmpSkillDataAsset;

// @Owner: 이정준 (구현: Codex) | 2026-10-30
// 임시 공통 베이스: DA 조회, 상태 제한, 구독 수명만 담당한다.
UCLASS(Abstract, Blueprintable)
class PROJECT_NF_API UNF_TmpSkillBase : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UNF_TmpSkillBase();
	UFUNCTION(BlueprintPure, Category = "Tmp Skill")
	const UNF_TmpSkillDataAsset* GetSkillData() const;
	virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
protected:
	virtual void HandleRestrictedState();
	bool IsStateBlocked() const;
private:
	void BindStateRules();
	void UnbindStateRules();
	void OnStateChanged(FGameplayTag Tag, int32 Count);
	TWeakObjectPtr<UAbilitySystemComponent> RuleASC;
	TMap<FGameplayTag, FDelegateHandle> RuleDelegates;
	UPROPERTY(Transient)
	TObjectPtr<UNF_TmpSkillDataAsset> SkillSource;
};
