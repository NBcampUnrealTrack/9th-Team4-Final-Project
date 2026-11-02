#include "GamePlay/Combat/Skill/Temporary/NF_TmpSkillBase.h"
#include "GamePlay/Combat/Skill/Temporary/NF_TmpSkillDataAsset.h"
#include "AbilitySystemComponent.h"

UNF_TmpSkillBase::UNF_TmpSkillBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

const UNF_TmpSkillDataAsset* UNF_TmpSkillBase::GetSkillData() const
{
	return SkillSource;
}

bool UNF_TmpSkillBase::IsStateBlocked() const
{
	const auto* Data = GetSkillData();
	const auto* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!Data || !Data->HasValidSettings() || !ASC) { return true; }
	for (const auto& Rule : Data->StateRules)
	{
		if (Rule.bBlockActivation && Rule.StateTag.IsValid() && ASC->HasMatchingGameplayTag(Rule.StateTag)) { return true; }
	}
	return false;
}

bool UNF_TmpSkillBase::CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	// GAS can ask the CDO before an instance is active; do not use instance-only accessors here.
	const auto* Data = Cast<UNF_TmpSkillDataAsset>(GetSourceObject(Handle, ActorInfo));
	const auto* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!Data || !Data->HasValidSettings() || !ASC) { return false; }
	for (const auto& Rule : Data->StateRules)
	{
		if (Rule.bBlockActivation && Rule.StateTag.IsValid() && ASC->HasMatchingGameplayTag(Rule.StateTag)) { return false; }
	}
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UNF_TmpSkillBase::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	SkillSource = Cast<UNF_TmpSkillDataAsset>(Spec.SourceObject.Get());
	Super::OnGiveAbility(ActorInfo, Spec); BindStateRules();
}
void UNF_TmpSkillBase::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec); BindStateRules();
}
void UNF_TmpSkillBase::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	UnbindStateRules(); Super::OnRemoveAbility(ActorInfo, Spec); SkillSource = nullptr;
}

void UNF_TmpSkillBase::BindStateRules()
{
	UnbindStateRules();
	auto* ASC = GetAbilitySystemComponentFromActorInfo();
	const auto* Data = GetSkillData();
	if (!ASC || !Data) { return; }
	RuleASC = ASC;
	for (const auto& Rule : Data->StateRules)
	{
		if (Rule.StateTag.IsValid() && !RuleDelegates.Contains(Rule.StateTag))
		{
			RuleDelegates.Add(Rule.StateTag, ASC->RegisterGameplayTagEvent(Rule.StateTag, EGameplayTagEventType::NewOrRemoved)
				.AddUObject(this, &ThisClass::OnStateChanged));
		}
	}
}

void UNF_TmpSkillBase::UnbindStateRules()
{
	if (auto* ASC = RuleASC.Get())
	{
		for (const auto& Entry : RuleDelegates)
		{
			ASC->RegisterGameplayTagEvent(Entry.Key, EGameplayTagEventType::NewOrRemoved).Remove(Entry.Value);
		}
	}
	RuleDelegates.Empty(); RuleASC.Reset();
}

void UNF_TmpSkillBase::OnStateChanged(FGameplayTag Tag, int32 Count)
{
	if (Count > 0) { HandleRestrictedState(); }
}

void UNF_TmpSkillBase::HandleRestrictedState()
{
	const auto* Info = GetCurrentActorInfo();
	const auto* Data = GetSkillData();
	const auto* ASC = RuleASC.Get();
	if (!Info || !Info->IsNetAuthority() || !IsActive() || !Data || !ASC) { return; }
	for (const auto& Rule : Data->StateRules)
	{
		if (Rule.bCancelRunning && Rule.StateTag.IsValid() && ASC->HasMatchingGameplayTag(Rule.StateTag)
			&& (Rule.bForceCancel || CanBeCanceled()))
		{
			EndAbility(CurrentSpecHandle, Info, CurrentActivationInfo, true, true); return;
		}
	}
}
