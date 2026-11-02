#include "GamePlay/Combat/Skill/Data/NF_SkillDataAsset.h"

#include "GamePlay/Combat/Skill/NFGA_SkillBase.h"
#include "AbilitySystemComponent.h"
#include "GamePlay/Combat/Skill/Preview/NF_SkillPreviewActor.h"
#include "GamePlay/Combat/Skill/Execution/NF_SkillExecutionActor.h"

bool UNF_SkillDataAsset::HasValidSettings() const
{
	if (!AbilityClass || AbilityClass->HasAnyClassFlags(CLASS_Abstract) || AbilityLevel < 1
		|| (bOverrideIdentity && BuildCost < 0)) { return false; }
	const auto* Defaults = AbilityClass->GetDefaultObject<UNFGA_SkillBase>();
	if (!Defaults) { return false; }
	if (IsA<UNF_ActiveSkillDataAsset>())
	{
		const auto* ActiveData = CastChecked<UNF_ActiveSkillDataAsset>(this);
		if (Defaults->GetSkillType() != ENF_SkillType::Active) { return false; }
		if (ActiveData->bOverrideActiveSettings && (!FMath::IsFinite(ActiveData->CastTime) || ActiveData->CastTime < 0.f
			|| !FMath::IsFinite(ActiveData->ExecutionDuration) || ActiveData->ExecutionDuration < 0.f
			|| (ActiveData->ExecutionMode == ENF_SkillExecutionMode::Timed && ActiveData->ExecutionDuration <= 0.f))) { return false; }
	}
	if (IsA<UNF_PassiveSkillDataAsset>() && Defaults->GetSkillType() != ENF_SkillType::Passive) { return false; }
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
	if (bOverrideIdentity)
	{
		Skill.SkillCategory = SkillCategory;
		Skill.BuildCost = BuildCost;
	}
	Skill.AdditionalStateRules = StateRules;
}

void UNF_ActiveSkillDataAsset::ConfigureInstance(UNFGA_SkillBase& Skill) const
{
	Super::ConfigureInstance(Skill);
	if (Skill.IsTemplate() || !HasValidSettings() || !Skill.IsA(AbilityClass.Get()) || !bOverrideActiveSettings) { return; }
	Skill.bActivateOnInputRelease = bActivateOnInputRelease;
	Skill.PreviewActorClass = PreviewActorClass;
	Skill.DefaultPreviewData = PreviewData;
	Skill.ExecutionActorClass = ExecutionActorClass;
	Skill.SkillExecutionMode = ExecutionMode;
	Skill.CastTime = CastTime;
	Skill.ExecutionDuration = ExecutionDuration;
	Skill.bRequiresConfirmedTargetData = bRequiresConfirmedTargetData;
}

void UNF_PassiveSkillDataAsset::ConfigureInstance(UNFGA_SkillBase& Skill) const
{
	Super::ConfigureInstance(Skill);
	if (Skill.IsTemplate() || !HasValidSettings() || !Skill.IsA(AbilityClass.Get()) || !bOverridePassiveSettings) { return; }
	Skill.PassiveEffects = PassiveEffects;
	Skill.PassiveEventTags = PassiveEventTags;
	Skill.bActivatePassiveOnGrant = bActivatePassiveOnGrant;
	Skill.ActivationCueTag = ActivationCueTag;
	Skill.ActiveCueTag = ActiveCueTag;
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
