#include "GamePlay/Combat/Skill/NFGA_SkillBase.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "GAS/Tags/NFTag_Player.h"
#include "GamePlay/Combat/Skill/NF_SkillRedirects.h"

UNFGA_SkillBase::UNFGA_SkillBase()
{
	NFRegisterSkillOwnershipRedirects();
	ActivationBlockedTags.AddTag(NFTag_Player::Player_State_Death);
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	NativeSkillDefaults = CreateDefaultSubobject<UNF_SkillDataAsset>(TEXT("NativeSkillDefaults"));
}

const UNF_SkillDataAsset* UNFGA_SkillBase::GetSkillData() const { return SkillData ? SkillData.Get() : SkillDefinition.Get(); }
UNF_SkillDataAsset& UNFGA_SkillBase::GetMutableSkillDefaults() { check(NativeSkillDefaults); return *NativeSkillDefaults; }
ENF_SkillType UNFGA_SkillBase::GetSkillType() const { return NativeSkillDefaults->SkillType; }
ENF_SkillCategory UNFGA_SkillBase::GetSkillCategory() const
{
	const auto* Data = GetSkillData();
	return Data && Data->bOverrideIdentity ? Data->SkillCategory : NativeSkillDefaults->SkillCategory;
}
int32 UNFGA_SkillBase::GetBuildCost() const
{
	const auto* Data = GetSkillData();
	return Data && Data->bOverrideIdentity ? Data->BuildCost : NativeSkillDefaults->BuildCost;
}
ENF_SkillActivationMethod UNFGA_SkillBase::GetSkillActivationMethod() const
{
	const auto* Data = GetSkillData();
	return Data && Data->bOverrideIdentity ? Data->SkillActivationMethod : NativeSkillDefaults->SkillActivationMethod;
}

void UNFGA_SkillBase::ConfigureFromSkillData(const FGameplayAbilitySpec& Spec)
{
	SkillData = Cast<UNF_SkillDataAsset>(Spec.SourceObject.Get());
	if (!SkillData) { SkillData = SkillDefinition; }
	AdditionalStateRules.Reset();
	if (SkillData) { SkillData->ConfigureInstance(*this); }
}

bool UNFGA_SkillBase::HasBlockingStateRule(const UAbilitySystemComponent* ASC) const
{
	if (!ASC) { return false; }
	for (const auto& Rule : AdditionalStateRules)
	{
		if (Rule.bCancelRunning && (Rule.bForceCancel || CanBeCanceled()) && Rule.StateTag.IsValid()
			&& ASC->HasMatchingGameplayTag(Rule.StateTag)) { return true; }
	}
	return false;
}

bool UNFGA_SkillBase::CanContinueSkillExecution() const
{
	const auto* ASC = GetAbilitySystemComponentFromActorInfo();
	return ASC && !ASC->HasMatchingGameplayTag(NFTag_Player::Player_State_Death)
		&& !HasBlockingStateRule(ASC) && !ASC->HasAnyMatchingGameplayTags(ActivationBlockedTags);
}

bool UNFGA_SkillBase::CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	const auto* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC || ASC->HasMatchingGameplayTag(NFTag_Player::Player_State_Death)) { return false; }
	const auto* Definition = Cast<UNF_SkillDataAsset>(GetSourceObject(Handle, ActorInfo));
	if (!Definition) { Definition = GetSkillData(); }
	if (Definition)
	{
		if (!Definition->HasValidSettings() || !IsA(Definition->AbilityClass.Get())) { return false; }
		for (const auto& Rule : Definition->StateRules)
		{
			if (Rule.bBlockActivation && Rule.StateTag.IsValid() && ASC->HasMatchingGameplayTag(Rule.StateTag)) { return false; }
		}
	}
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UNFGA_SkillBase::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	ConfigureFromSkillData(Spec);
	Super::OnGiveAbility(ActorInfo, Spec);
	BindCancellationTags();
}

void UNFGA_SkillBase::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	CancelLocalInput();
	AActor* NewAvatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (RegisteredAvatar.Get() != NewAvatar)
	{
		if (IsActive()) { EndAbility(Spec.Handle, ActorInfo, CurrentActivationInfo, true, true); }
		ClearConfirmedTargetData();
	}
	RegisteredAvatar = NewAvatar;
	Super::OnAvatarSet(ActorInfo, Spec);
	BindCancellationTags();
}

void UNFGA_SkillBase::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	UnbindCancellationTags();
	Super::OnRemoveAbility(ActorInfo, Spec);
	SkillData = nullptr;
	AdditionalStateRules.Reset();
}

void UNFGA_SkillBase::BindCancellationTags()
{
	UnbindCancellationTags();
	auto* ASC = GetAbilitySystemComponentFromActorInfo();
	RegisteredASC = ASC;
	if (!ASC) { return; }
	FGameplayTagContainer Tags = ActivationBlockedTags;
	Tags.AddTag(NFTag_Player::Player_State_Death);
	for (const auto& Rule : AdditionalStateRules) { if (Rule.StateTag.IsValid()) { Tags.AddTag(Rule.StateTag); } }
	CollectStateRestrictionTags(Tags);
	for (const auto& Tag : Tags)
	{
		CancellationDelegates.Add(Tag, ASC->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UNFGA_SkillBase::OnCancellationTagChanged));
	}
}

void UNFGA_SkillBase::UnbindCancellationTags()
{
	if (auto* ASC = RegisteredASC.Get())
	{
		for (const auto& Entry : CancellationDelegates)
		{
			ASC->RegisterGameplayTagEvent(Entry.Key, EGameplayTagEventType::NewOrRemoved).Remove(Entry.Value);
		}
	}
	CancellationDelegates.Empty();
	RegisteredASC.Reset();
}

void UNFGA_SkillBase::OnCancellationTagChanged(FGameplayTag Tag, int32 NewCount)
{
	if (NewCount <= 0) { return; }
	const auto Rules = AdditionalStateRules;
	for (const auto& Rule : Rules)
	{
		if (!Rule.StateTag.IsValid() || Rule.StateTag != Tag) { continue; }
		if (Rule.bBlockActivation) { CancelLocalInput(); }
		if (Rule.bCancelRunning && IsActive() && (Rule.bForceCancel || CanBeCanceled()))
		{
			EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true, true);
			break;
		}
	}
	if (ActivationBlockedTags.HasTagExact(Tag) || Tag == NFTag_Player::Player_State_Death)
	{
		CancelLocalInput();
		if (IsActive() && CanBeCanceled()) { CancelAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true); }
	}
	HandleStateRestrictionChanged(Tag, NewCount);
}

bool UNFGA_SkillBase::ConfirmLocalInput(FGameplayAbilityTargetDataHandle& OutTargetData)
{
	OutTargetData.Clear(); return false;
}

bool UNFGA_SkillBase::UsesDefinitionStateRules() const
{
	const auto* Data = GetSkillData();
	return Data && Data->bOverrideStateRules;
}

FGameplayAbilityTargetDataHandle UNFGA_SkillBase::MakeSkillTargetData(ENF_SkillTargetKind Kind,
	FVector Location, FVector AimDirection)
{
	FGameplayAbilityTargetDataHandle Result;
	FNF_SkillTargetData* Data = new FNF_SkillTargetData();
	Data->Kind = Kind;
	Data->Location = Location;
	Data->AimDirection = AimDirection.GetSafeNormal();
	Result.Add(Data);
	return Result;
}

bool UNFGA_SkillBase::ReadSkillTargetData(const FGameplayAbilityTargetDataHandle& TargetData,
	ENF_SkillTargetKind& Kind, FVector& Location, FVector& AimDirection)
{
	Kind = ENF_SkillTargetKind::None;
	Location = FVector::ZeroVector;
	AimDirection = FVector::ZeroVector;
	const FNF_SkillTargetData* Data = FNF_SkillTargetData::Get(TargetData);
	if (!Data) { return false; }
	Kind = Data->Kind;
	Location = Data->Location;
	AimDirection = Data->AimDirection;
	return true;
}
