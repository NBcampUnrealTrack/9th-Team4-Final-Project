#include "GamePlay/Combat/Skill/NFGA_SkillBase.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameFramework/Actor.h"
#include "Project_NF.h"

// 호환 구현: 기존 함수/속성 소유 클래스는 유지하고 구현만 역할별로 분리한다.
namespace
{
	// @Owner: 스킬 담당 (구현: Codex) | 2026-10-13
	// UE 5.7의 GetStackingType은 DLL export가 없어 프로젝트 모듈에서 링크할 수 없다.
	// 공개 필드를 읽는 호환 처리를 이 함수로 한정한다. 엔진 소스는 수정하지 않는다.
	bool HasSharedStacking(const UGameplayEffect& Effect)
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		const bool bHasSharedStacking = Effect.StackingType != EGameplayEffectStackingType::None;
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		return bHasSharedStacking;
	}
}

void UNFGA_SkillBase::InitializePassive()
{
	const FGameplayAbilityActorInfo* Info = GetCurrentActorInfo();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (SkillType != ENF_SkillType::Passive || bPassiveInitialized || !Info || !Info->IsNetAuthority()
		|| !ASC || !IsValid(Avatar)) { return; }
	const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(CurrentSpecHandle);
	if (!Spec || Spec->GetPrimaryInstance() != this) { return; }
	bPassiveInitialized = true;
	PassiveAvatar = Avatar;
	RegisteredASC = ASC;
	RegisteredPassiveEventTags = PassiveEventTags;
	if (!RegisteredPassiveEventTags.IsEmpty())
	{
		PassiveEventDelegate = ASC->AddGameplayEventTagContainerDelegate(RegisteredPassiveEventTags,
			FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &UNFGA_SkillBase::HandlePassiveGameplayEvent));
	}
	for (const TSubclassOf<UGameplayEffect>& Effect : PassiveEffects)
	{
		if (!bPassiveInitialized) { break; }
		if (!Effect) { continue; }
		const UGameplayEffect* Defaults = Effect->GetDefaultObject<UGameplayEffect>();
		if (Defaults->DurationPolicy == EGameplayEffectDurationType::Instant || HasSharedStacking(*Defaults))
		{
			UE_LOG(LogProject_NF, Warning, TEXT("[Skill] Passive Effects require Duration/Infinite and Stacking Type None: %s"), *GetNameSafe(Effect.Get()));
			continue;
		}
		const FGameplayEffectSpecHandle EffectSpec = MakeOutgoingGameplayEffectSpec(Effect, GetAbilityLevel());
		if (EffectSpec.IsValid())
		{
			const FActiveGameplayEffectHandle Applied = ASC->ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());
			if (bPassiveInitialized) { TrackPassiveEffect(Applied); }
			else { ASC->RemoveActiveGameplayEffect(Applied); }
		}
	}
	if (bPassiveInitialized) { PassiveEquipped(); }
	if (bPassiveInitialized && bActivatePassiveOnGrant)
	{
		ASC->TryActivateAbility(CurrentSpecHandle, false);
	}
}

void UNFGA_SkillBase::TrackPassiveEffect(FActiveGameplayEffectHandle EffectHandle)
{
	UAbilitySystemComponent* ASC = RegisteredASC.Get();
	const FActiveGameplayEffect* Effect = ASC ? ASC->GetActiveGameplayEffect(EffectHandle) : nullptr;
	if (bPassiveInitialized && ASC && ASC->IsOwnerActorAuthoritative() && Effect && Effect->Spec.Def
		&& !HasSharedStacking(*Effect->Spec.Def))
	{
		PassiveEffectHandles.AddUnique(EffectHandle);
	}
}

void UNFGA_SkillBase::CleanupPassive()
{
	StopPassiveActiveCue();
	if (!bPassiveInitialized) { return; }
	bPassiveInitialized = false;
	UAbilitySystemComponent* ASC = RegisteredASC.Get();
	if (ASC && PassiveEventDelegate.IsValid())
	{
		ASC->RemoveGameplayEventTagContainerDelegate(RegisteredPassiveEventTags, PassiveEventDelegate);
	}
	PassiveEventDelegate.Reset();
	RegisteredPassiveEventTags.Reset();
	TArray<FActiveGameplayEffectHandle> Handles = MoveTemp(PassiveEffectHandles);
	if (ASC)
	{
		for (FActiveGameplayEffectHandle Handle : Handles) { ASC->RemoveActiveGameplayEffect(Handle); }
	}
	PassiveAvatar.Reset();
	PassiveUnequipped();
}

void UNFGA_SkillBase::HandlePassiveGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload)
{
	if (bPassiveInitialized && Payload && CanContinueSkillExecution()) { PassiveGameplayEvent(EventTag, *Payload); }
}

void UNFGA_SkillBase::NotifyPassiveTriggered(AActor* AffectedActor, FVector Location, float Magnitude)
{
	const auto* Info = GetCurrentActorInfo();
	auto* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!Info || !Info->IsNetAuthority() || !ASC || SkillType != ENF_SkillType::Passive || !ActivationCueTag.IsValid()) { return; }
	FGameplayCueParameters Parameters;
	Parameters.EffectContext = ASC->MakeEffectContext();
	Parameters.Instigator = GetAvatarActorFromActorInfo();
	Parameters.EffectCauser = GetAvatarActorFromActorInfo();
	Parameters.SourceObject = AffectedActor;
	Parameters.Location = Location;
	Parameters.RawMagnitude = Magnitude;
	Parameters.AbilityLevel = GetAbilityLevel();
	// Execute on the caster's ASC: cue target is the passive owner, SourceObject is the affected actor.
	ASC->ExecuteGameplayCue(ActivationCueTag, Parameters);
}

void UNFGA_SkillBase::StartPassiveActiveCue()
{
	const auto* Info = GetCurrentActorInfo();
	auto* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!Info || !Info->IsNetAuthority() || !ASC || SkillType != ENF_SkillType::Passive || !ActiveCueTag.IsValid()) { return; }
	if (PassiveCueASC.Get() == ASC && RegisteredActiveCueTag == ActiveCueTag) { return; }
	StopPassiveActiveCue();
	FGameplayCueParameters Parameters;
	Parameters.EffectContext = ASC->MakeEffectContext();
	Parameters.Instigator = GetAvatarActorFromActorInfo();
	Parameters.EffectCauser = GetAvatarActorFromActorInfo();
	if (const auto* Avatar = GetAvatarActorFromActorInfo()) { Parameters.Location = Avatar->GetActorLocation(); }
	// Store ownership before invoking cue callbacks, which may synchronously unequip the passive.
	PassiveCueASC = ASC; RegisteredActiveCueTag = ActiveCueTag;
	ASC->AddGameplayCue(ActiveCueTag, Parameters);
}

void UNFGA_SkillBase::StopPassiveActiveCue()
{
	auto* ASC = PassiveCueASC.Get();
	const auto Tag = RegisteredActiveCueTag;
	PassiveCueASC.Reset(); RegisteredActiveCueTag = FGameplayTag();
	if (ASC && Tag.IsValid()) { ASC->RemoveGameplayCue(Tag); }
}
