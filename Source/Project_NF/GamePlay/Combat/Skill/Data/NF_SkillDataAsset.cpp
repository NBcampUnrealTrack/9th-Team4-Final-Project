#include "GamePlay/Combat/Skill/Data/NF_SkillDataAsset.h"

#include "GamePlay/Combat/Skill/NFGA_SkillBase.h"
#include "GamePlay/Combat/Skill/Active/NFGA_ActiveSkillBase.h"
#include "GamePlay/Combat/Skill/Passive/NFGA_PassiveSkillBase.h"
#include "AbilitySystemComponent.h"
#include "GamePlay/Combat/Skill/Preview/NF_SkillPreviewActor.h"
#include "GamePlay/Combat/Skill/Execution/NF_SkillExecutionActor.h"
#include "Animation/AnimMontage.h"

UNF_ActiveSkillDataAsset::UNF_ActiveSkillDataAsset() { SkillType = ENF_SkillType::Active; }
UNF_PassiveSkillDataAsset::UNF_PassiveSkillDataAsset() { SkillType = ENF_SkillType::Passive; SkillActivationMethod = ENF_SkillActivationMethod::OnGranted; }

bool UNF_SkillDataAsset::HasValidSettings() const
{
	if (!AbilityClass || AbilityClass->HasAnyClassFlags(CLASS_Abstract) || AbilityLevel < 1
		|| (bOverrideIdentity && BuildCost < 0)) { return false; }
	const auto* Defaults = AbilityClass->GetDefaultObject<UNFGA_SkillBase>();
	if (!Defaults) { return false; }
	if (IsA<UNF_ActiveSkillDataAsset>())
	{
		const auto* ActiveData = CastChecked<UNF_ActiveSkillDataAsset>(this);
		if (!AbilityClass->IsChildOf(UNFGA_ActiveSkillBase::StaticClass()) || Defaults->GetSkillType() != ENF_SkillType::Active) { return false; }
		if (ActiveData->bOverrideActiveSettings && (!FMath::IsFinite(ActiveData->CastTime) || ActiveData->CastTime < 0.f
			|| !FMath::IsFinite(ActiveData->ExecutionDuration) || ActiveData->ExecutionDuration < 0.f
			|| (ActiveData->ExecutionMode == ENF_SkillExecutionMode::Timed && ActiveData->ExecutionDuration <= 0.f))) { return false; }
	}
	if (IsA<UNF_PassiveSkillDataAsset>() && (!AbilityClass->IsChildOf(UNFGA_PassiveSkillBase::StaticClass())
		|| Defaults->GetSkillType() != ENF_SkillType::Passive)) { return false; }
	return true;
}

FGameplayAbilitySpecHandle UNF_SkillDataAsset::GrantTo(UAbilitySystemComponent* ASC, int32 InputID) const
{
	if (!IsValid(ASC) || !ASC->IsOwnerActorAuthoritative() || !HasValidSettings()) { return {}; }
	return ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, AbilityLevel, InputID, const_cast<UNF_SkillDataAsset*>(this)));
}

void UNF_SkillDataAsset::ConfigureInstance(UNFGA_SkillBase& Skill) const
{
	if (Skill.IsTemplate() || !HasValidSettings() || !Skill.IsA(AbilityClass.Get())) { return; }
	Skill.AdditionalStateRules = StateRules;
}

void UNF_ActiveSkillDataAsset::ConfigureInstance(UNFGA_SkillBase& Skill) const
{
	Super::ConfigureInstance(Skill);
	if (Skill.IsTemplate() || !HasValidSettings() || !Skill.IsA(AbilityClass.Get()) || !bOverrideActiveSettings) { return; }
	auto* Active = Cast<UNFGA_ActiveSkillBase>(&Skill);
	if (!Active) { return; }
	Active->bActivateOnInputRelease = bActivateOnInputRelease;
	Active->PreviewActorClass = PreviewActorClass;
	Active->DefaultPreviewData = PreviewData;
	Active->ExecutionActorClass = ExecutionActorClass;
	Active->SkillExecutionMode = ExecutionMode;
	Active->CastTime = CastTime;
	Active->ExecutionDuration = ExecutionDuration;
	Active->bRequiresConfirmedTargetData = bRequiresConfirmedTargetData;
	Active->CastMontage = CastMontage;
	Active->CastMontagePlayRate = CastMontagePlayRate;
	Active->CastMontageStartSection = CastMontageStartSection;
}

void UNF_PassiveSkillDataAsset::ConfigureInstance(UNFGA_SkillBase& Skill) const
{
	Super::ConfigureInstance(Skill);
	if (Skill.IsTemplate() || !HasValidSettings() || !Skill.IsA(AbilityClass.Get()) || !bOverridePassiveSettings) { return; }
	auto* Passive = Cast<UNFGA_PassiveSkillBase>(&Skill);
	if (!Passive) { return; }
	Passive->PassiveEffects = PassiveEffects;
	Passive->PassiveEventTags = PassiveEventTags;
	Passive->bActivatePassiveOnGrant = bActivatePassiveOnGrant;
	Passive->ActivationCueTag = ActivationCueTag;
	Passive->ActiveCueTag = ActiveCueTag;
}

bool UNF_SkillDataAsset::GetBuildItemData_Implementation(FNF_BuildItemData& OutData) const
{
	OutData = FNF_BuildItemData();
	if (!HasValidSettings()) { return false; }
	const UNFGA_SkillBase* Defaults = AbilityClass->GetDefaultObject<UNFGA_SkillBase>();
	if (!IsValid(Defaults) || Defaults->GetBuildCost() < 0) { return false; }

	FNF_BuildItemData Data;
	switch (Defaults->GetSkillType())
	{
	case ENF_SkillType::Active: Data.ItemType = ENF_ItemType::ActiveSkill; break;
	case ENF_SkillType::Passive: Data.ItemType = ENF_ItemType::PassiveSkill; break;
	default: return false;
	}
	Data.Cost = bOverrideIdentity ? BuildCost : Defaults->GetBuildCost();
	Data.AbilityClass = AbilityClass;
	Data.AbilityLevel = AbilityLevel;
	OutData = MoveTemp(Data);
	return true;
}
