#include "GamePlay/Combat/Skill/Temporary/NF_TmpPassiveSkillBase.h"
#include "GamePlay/Combat/Skill/Temporary/NF_TmpSkillDataAsset.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "Project_NF.h"
#include "TimerManager.h"

void UNF_TmpPassiveSkillBase::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	bRemoving = false; Super::OnGiveAbility(ActorInfo, Spec); ScheduleActivation();
}
void UNF_TmpPassiveSkillBase::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	if (IsActive()) { EndAbility(Spec.Handle, ActorInfo, CurrentActivationInfo, true, true); }
	Super::OnAvatarSet(ActorInfo, Spec); ScheduleActivation();
}
void UNF_TmpPassiveSkillBase::ScheduleActivation()
{
	const auto* Info = GetCurrentActorInfo();
	if (bRemoving || !Info || !Info->IsNetAuthority() || !GetWorld()) { return; }
	GetWorld()->GetTimerManager().ClearTimer(GrantTimer);
	GrantTimer = GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::TryActivateEquipped);
}
void UNF_TmpPassiveSkillBase::TryActivateEquipped()
{
	auto* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!bRemoving && ASC && !IsActive()) { ASC->TryActivateAbility(CurrentSpecHandle, false); }
}
void UNF_TmpPassiveSkillBase::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	bRemoving = true;
	if (GetWorld()) { GetWorld()->GetTimerManager().ClearTimer(GrantTimer); }
	Cleanup(); Super::OnRemoveAbility(ActorInfo, Spec);
}

void UNF_TmpPassiveSkillBase::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const auto* Data = Cast<UNF_TmpPassiveSkillDataAsset>(GetSkillData());
	auto* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ActorInfo || !ActorInfo->IsNetAuthority() || !Data || !ASC || IsStateBlocked())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return;
	}
	bEquipped = true; EffectASC = ASC;
	BoundEventTags = Data->EventTags;
	if (!BoundEventTags.IsEmpty())
	{
		EventHandle = ASC->AddGameplayEventTagContainerDelegate(BoundEventTags,
			FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::HandleEvent));
	}
	const auto Effects = Data->EquippedEffects;
	for (const auto& Class : Effects)
	{
		if (!bEquipped || !IsActive()) { break; }
		if (!Class) { continue; }
		const auto* Def = Class->GetDefaultObject<UGameplayEffect>();
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		const bool bSharedStack = Def->StackingType != EGameplayEffectStackingType::None;
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		if (Def->DurationPolicy == EGameplayEffectDurationType::Instant || bSharedStack)
		{
			UE_LOG(LogProject_NF, Warning, TEXT("[TmpSkill] 장착 GE는 Duration/Infinite, No Stacking만 지원합니다: %s"), *GetNameSafe(Class.Get())); continue;
		}
		const auto EffectSpec = MakeOutgoingGameplayEffectSpec(Class, GetAbilityLevel());
		if (!EffectSpec.IsValid()) { continue; }
		const auto Applied = ASC->ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());
		if (bEquipped && IsActive()) { EffectHandles.Add(Applied); }
		else { ASC->RemoveActiveGameplayEffect(Applied); }
	}
	if (bEquipped && IsActive()) { Equipped(); }
}

void UNF_TmpPassiveSkillBase::HandleEvent(FGameplayTag Tag, const FGameplayEventData* Payload)
{
	if (bEquipped && IsActive() && !IsStateBlocked() && Payload) { PassiveEvent(Tag, *Payload); }
}
void UNF_TmpPassiveSkillBase::NotifyTriggered(float Magnitude)
{
	const auto* Info = GetCurrentActorInfo();
	const auto* Data = Cast<UNF_TmpPassiveSkillDataAsset>(GetSkillData());
	auto* ASC = EffectASC.Get();
	if (!Info || !Info->IsNetAuthority() || !bEquipped || !IsActive() || IsStateBlocked()
		|| !Data || !ASC || !Data->TriggerCueTag.IsValid() || !FMath::IsFinite(Magnitude)) { return; }
	FGameplayCueParameters Params;
	Params.EffectContext = ASC->MakeEffectContext(); Params.RawMagnitude = Magnitude;
	Params.Instigator = GetAvatarActorFromActorInfo(); Params.EffectCauser = GetAvatarActorFromActorInfo();
	ASC->ExecuteGameplayCue(Data->TriggerCueTag, Params);
}
void UNF_TmpPassiveSkillBase::Cleanup()
{
	if (!bEquipped) { return; }
	bEquipped = false;
	auto* ASC = EffectASC.Get(); EffectASC.Reset();
	const auto OldTags = BoundEventTags; BoundEventTags.Reset();
	const auto OldEvent = EventHandle; EventHandle.Reset();
	auto Handles = MoveTemp(EffectHandles);
	if (ASC)
	{
		if (OldEvent.IsValid()) { ASC->RemoveGameplayEventTagContainerDelegate(OldTags, OldEvent); }
		for (const auto Handle : Handles) { ASC->RemoveActiveGameplayEffect(Handle); }
	}
	Unequipped();
}
void UNF_TmpPassiveSkillBase::EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (!IsEndAbilityValid(Handle, ActorInfo)) { return; }
	if (ScopeLockCount > 0) { Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled); return; }
	Cleanup(); Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
