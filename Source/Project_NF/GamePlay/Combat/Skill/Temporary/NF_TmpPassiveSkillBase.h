#pragma once
#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/Temporary/NF_TmpSkillBase.h"
#include "NF_TmpPassiveSkillBase.generated.h"

// @Owner: 이정준 (구현: Codex) | 2026-10-30 | 패시브 장착 효과·이벤트·연출만 관리.
UCLASS(Blueprintable)
class PROJECT_NF_API UNF_TmpPassiveSkillBase : public UNF_TmpSkillBase
{
	GENERATED_BODY()
public:
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	// 조건이 성공한 후 자식이 호출한다. 피해나 스탯 변경을 대신하지 않는다.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Tmp Skill|Passive")
	void NotifyTriggered(float Magnitude = 0.f);
protected:
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	UFUNCTION(BlueprintImplementableEvent, Category = "Tmp Skill|Passive")
	void Equipped();
	UFUNCTION(BlueprintImplementableEvent, Category = "Tmp Skill|Passive")
	void Unequipped();
	UFUNCTION(BlueprintImplementableEvent, Category = "Tmp Skill|Passive")
	void PassiveEvent(FGameplayTag Tag, const FGameplayEventData& Payload);
private:
	void ScheduleActivation();
	void TryActivateEquipped();
	void Cleanup();
	void HandleEvent(FGameplayTag Tag, const FGameplayEventData* Payload);
	TArray<FActiveGameplayEffectHandle> EffectHandles;
	TWeakObjectPtr<UAbilitySystemComponent> EffectASC;
	FGameplayTagContainer BoundEventTags;
	FDelegateHandle EventHandle;
	FTimerHandle GrantTimer;
	bool bEquipped = false;
	bool bRemoving = false;
};
