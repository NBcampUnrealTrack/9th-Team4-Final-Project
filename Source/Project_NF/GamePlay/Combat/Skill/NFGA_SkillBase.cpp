#include "GamePlay/Combat/Skill/NFGA_SkillBase.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "GAS/Tags/NFTag_Skill.h"
#include "GAS/Tags/NFTag_Player.h"
#include "GamePlay/Combat/Skill/Data/NF_SkillDataAsset.h"

// @Owner: 스킬 담당 | 2026-10-12
UNFGA_SkillBase::UNFGA_SkillBase()
{
	// @Owner: 이정준 (구현: Codex) | 2026-10-22
	// Inherited by every player skill; the death ability itself does not derive from this base.
	ActivationBlockedTags.AddTag(NFTag_Player::Player_State_Death);
	// 각 ASC의 스킬 인스턴스에서 실행 상태를 보관한다.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 초기 공통 베이스는 서버에서 실행한다. 예측이 필요한 스킬은 자식에서 정책을 설정한다.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	DefaultPreviewData.DisplayRadius = 300.0f;
}

void UNFGA_SkillBase::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	CancelLocalInput();
	ClearConfirmedTargetData();
	CleanupPassive();
	UnbindCancellationTags();
	Super::OnRemoveAbility(ActorInfo, Spec);
}

bool UNFGA_SkillBase::CanContinueSkillExecution() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	return ASC && !HasBlockingStateRule(ASC, true) && !ASC->HasMatchingGameplayTag(NFTag_Player::Player_State_Death)
		&& (!bCancelOnEMP || !IsBlockedByEMP()) && !ASC->HasAnyMatchingGameplayTags(InputCancelTags)
		&& !ASC->HasAnyMatchingGameplayTags(ActivationBlockedTags);
}

bool UNFGA_SkillBase::IsBlockedByEMP() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	return IsAffectedByEMP() && ASC && ASC->HasMatchingGameplayTag(NFTag_Skill::State_Debuff_EMP);
}

bool UNFGA_SkillBase::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	// CDO preflight must read the spec source rather than instance-only state.
	const auto* Definition = Cast<UNF_SkillDataAsset>(GetSourceObject(Handle, ActorInfo));
	if (Definition)
	{
		if (!Definition->HasValidSettings()) { return false; }
		for (const auto& Rule : Definition->StateRules)
		{
			if (ASC && Rule.bBlockActivation && Rule.StateTag.IsValid() && ASC->HasMatchingGameplayTag(Rule.StateTag)) { return false; }
		}
	}
	if (ExecutionActorClass && (NetExecutionPolicy != EGameplayAbilityNetExecutionPolicy::ServerOnly
		|| !FMath::IsFinite(CastTime) || CastTime < 0.0f || !FMath::IsFinite(ExecutionDuration)
		|| (SkillExecutionMode == ENF_SkillExecutionMode::Timed && ExecutionDuration <= 0.0f))) { return false; }
	// Also enforce death explicitly: an old BP can override/clear the inherited editable tag container.
	return ASC && !ASC->HasMatchingGameplayTag(NFTag_Player::Player_State_Death)
		&& (!IsAffectedByEMP() || !ASC->HasMatchingGameplayTag(NFTag_Skill::State_Debuff_EMP))
		&& !ASC->HasAnyMatchingGameplayTags(InputCancelTags)
		&& Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UNFGA_SkillBase::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	ConfigureFromSkillData(Spec);
	Super::OnGiveAbility(ActorInfo, Spec);
	BindCancellationTags();
	InitializePassive();
}

void UNFGA_SkillBase::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	CancelLocalInput();
	AActor* NewAvatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (RegisteredAvatar.Get() != NewAvatar)
	{
		// 이전 Pawn에서 준비 중이던 스킬이 새 Pawn에서 실행되지 않게 종료한다.
		if (IsActive()) { EndAbility(Spec.Handle, ActorInfo, CurrentActivationInfo, true, true); }
		ClearConfirmedTargetData();
	}
	RegisteredAvatar = NewAvatar;
	Super::OnAvatarSet(ActorInfo, Spec);
	if (bPassiveInitialized && PassiveAvatar.Get() != GetAvatarActorFromActorInfo())
	{
		CleanupPassive();
	}
	BindCancellationTags();
	InitializePassive();
}

void UNFGA_SkillBase::BindCancellationTags()
{
	UnbindCancellationTags();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	RegisteredASC = ASC;
	if (!ASC) { return; }
	FGameplayTagContainer Tags = InputCancelTags;
	for (const auto& Rule : AdditionalStateRules)
	{
		if (Rule.StateTag.IsValid()) { Tags.AddTag(Rule.StateTag); }
	}
	Tags.AppendTags(ActivationBlockedTags);
	Tags.AddTag(NFTag_Player::Player_State_Death);
	if (IsAffectedByEMP()) { Tags.AddTag(NFTag_Skill::State_Debuff_EMP); }
	for (const FGameplayTag& Tag : Tags)
	{
		CancellationDelegates.Add(Tag, ASC->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UNFGA_SkillBase::OnCancellationTagChanged));
	}
	if (IsBlockedByEMP()) { InterruptForEMP(); }
}

void UNFGA_SkillBase::UnbindCancellationTags()
{
	if (UAbilitySystemComponent* ASC = RegisteredASC.Get())
	{
		for (const auto& Entry : CancellationDelegates)
		{
			ASC->RegisterGameplayTagEvent(Entry.Key, EGameplayTagEventType::NewOrRemoved).Remove(Entry.Value);
		}
	}
	CancellationDelegates.Empty();
}

void UNFGA_SkillBase::OnCancellationTagChanged(FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		for (const auto& Rule : AdditionalStateRules)
		{
			if (!Rule.StateTag.IsValid() || Rule.StateTag != Tag) { continue; }
			if (Rule.bBlockActivation) { CancelLocalInput(); }
			if (Rule.bCancelRunning && IsActive() && (Rule.bForceCancel || CanBeCanceled()))
			{
				EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true, true);
				break;
			}
		}
	}
	if (Tag == NFTag_Skill::State_Debuff_EMP)
	{
		if (NewCount > 0 && IsAffectedByEMP()) { InterruptForEMP(); }
		return;
	}
	// Additional rules must not fall through to the legacy unconditional cancellation path.
	if (!InputCancelTags.HasTagExact(Tag) && !ActivationBlockedTags.HasTagExact(Tag)
		&& Tag != NFTag_Player::Player_State_Death) { return; }
	if (NewCount > 0)
	{
		CancelLocalInput();
		if (IsActive() && CanBeCanceled())
		{
			CancelAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true);
		}
	}
}

void UNFGA_SkillBase::InterruptForEMP()
{
	CancelLocalInput();
	if (!IsActive() || bCancelOnEMP) { ClearConfirmedTargetData(); }
	if (IsActive() && bCancelOnEMP)
	{
		// EMP is a forced interruption. Do not leave non-cancellable casting/maintained effects alive.
		// Virtual EndAbility invokes each skill's cleanup, including its timers and reveal requests.
		EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true, true);
	}
}

const UNF_SkillDataAsset* UNFGA_SkillBase::GetSkillData() const
{
	return SkillData;
}

void UNFGA_SkillBase::ConfigureFromSkillData(const FGameplayAbilitySpec& Spec)
{
	// Legacy build/boss sources are not skill DAs; their saved GA settings remain unchanged.
	SkillData = Cast<UNF_SkillDataAsset>(Spec.SourceObject.Get());
	AdditionalStateRules.Reset();
	if (SkillData) { SkillData->ConfigureInstance(*this); }
}

bool UNFGA_SkillBase::HasBlockingStateRule(const UAbilitySystemComponent* ASC, bool bRunning) const
{
	if (!ASC) { return false; }
	for (const auto& Rule : AdditionalStateRules)
	{
		if ((bRunning ? (Rule.bCancelRunning && (Rule.bForceCancel || CanBeCanceled())) : Rule.bBlockActivation) && Rule.StateTag.IsValid()
			&& ASC->HasMatchingGameplayTag(Rule.StateTag)) { return true; }
	}
	return false;
}
