#pragma once

#include "GamePlay/Combat/Skill/NFGA_SkillBase.h"
#include "NFGA_PassiveSkillBase.generated.h"

UCLASS(Blueprintable, HideCategories = ("NF|Skill|Input", "NF|Skill|Targeting"))
class PROJECT_NF_API UNFGA_PassiveSkillBase : public UNFGA_SkillBase
{
	GENERATED_BODY()
public:
	UNFGA_PassiveSkillBase();
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Skill|Passive")
	void TrackPassiveEffect(FActiveGameplayEffectHandle EffectHandle);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Skill|Passive|Visual")
	void NotifyPassiveTriggered(AActor* AffectedActor, FVector Location, float Magnitude);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Skill|Passive|Visual")
	void StartPassiveActiveCue();
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Skill|Passive|Visual")
	void StopPassiveActiveCue();
protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "NF|Skill|Passive")
	void PassiveEquipped();
	UFUNCTION(BlueprintImplementableEvent, Category = "NF|Skill|Passive")
	void PassiveUnequipped();
	UFUNCTION(BlueprintImplementableEvent, Category = "NF|Skill|Passive")
	void PassiveGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Passive|Visual", meta = (Categories = "GameplayCue"))
	FGameplayTag ActivationCueTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Passive|Visual", meta = (Categories = "GameplayCue"))
	FGameplayTag ActiveCueTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Passive")
	TArray<TSubclassOf<UGameplayEffect>> PassiveEffects;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Passive")
	FGameplayTagContainer PassiveEventTags;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Passive")
	bool bActivatePassiveOnGrant = false;
private:
	friend class UNF_PassiveSkillDataAsset;
	friend class FNF_BerserkerTest;
	void InitializePassive();
	void CleanupPassive();
	void HandlePassiveGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload);
	bool bPassiveInitialized = false;
	FDelegateHandle PassiveEventDelegate;
	FGameplayTagContainer RegisteredPassiveEventTags;
	TWeakObjectPtr<UAbilitySystemComponent> PassiveASC;
	TWeakObjectPtr<AActor> PassiveAvatar;
	TArray<FActiveGameplayEffectHandle> PassiveEffectHandles;
	TWeakObjectPtr<UAbilitySystemComponent> PassiveCueASC;
	FGameplayTag RegisteredActiveCueTag;
};
