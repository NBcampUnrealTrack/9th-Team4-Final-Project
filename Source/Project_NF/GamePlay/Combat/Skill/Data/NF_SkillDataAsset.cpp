#include "GamePlay/Combat/Skill/Data/NF_SkillDataAsset.h"

#include "AbilitySystemComponent.h"
#include "GamePlay/Combat/Skill/Active/NFGA_ActiveSkillBase.h"
#include "GamePlay/Combat/Skill/Passive/NFGA_PassiveSkillBase.h"
#include "GamePlay/Combat/Skill/Execution/NF_SkillExecutionActor.h"
#include "GAS/Tags/NFTag_Skill.h"

UNF_ActiveSkillDataAsset::UNF_ActiveSkillDataAsset()
{
	SkillType = ENF_SkillType::Active;
	PreviewData.DisplayRadius = 300.f;
	FNF_SkillStateRule EMP;
	EMP.StateTag = NFTag_Skill::State_Debuff_EMP;
	EMP.bForceCancel = true;
	StateRules.Add(EMP);
}

UNF_PassiveSkillDataAsset::UNF_PassiveSkillDataAsset()
{
	SkillType = ENF_SkillType::Passive;
	SkillActivationMethod = ENF_SkillActivationMethod::OnGranted;
}

bool UNF_SkillDataAsset::HasValidSettings() const
{
	if (!AbilityClass || AbilityClass->HasAnyClassFlags(CLASS_Abstract) || AbilityLevel < 1 || BuildCost < 0) { return false; }
	if (const auto* Active = Cast<UNF_ActiveSkillDataAsset>(this))
	{
		return AbilityClass->IsChildOf(UNFGA_ActiveSkillBase::StaticClass())
			&& FMath::IsFinite(Active->CastTime) && Active->CastTime >= 0.f
			&& FMath::IsFinite(Active->ExecutionDuration) && Active->ExecutionDuration >= 0.f
			&& FMath::IsFinite(Active->CastMontagePlayRate) && Active->CastMontagePlayRate > 0.f
			&& (!Active->ExecutionActorClass || Active->ExecutionMode != ENF_SkillExecutionMode::Timed || Active->ExecutionDuration > 0.f);
	}
	if (IsA<UNF_PassiveSkillDataAsset>()) { return AbilityClass->IsChildOf(UNFGA_PassiveSkillBase::StaticClass()); }
	// An old common-only DA has no complete execution definition. Reject it instead of treating missing cost/settings as zero.
	return false;
}

FGameplayAbilitySpecHandle UNF_SkillDataAsset::GrantTo(UAbilitySystemComponent* ASC, int32 InputID) const
{
	if (!IsValid(ASC) || !ASC->IsOwnerActorAuthoritative() || !HasValidSettings()
		|| (!IsA<UNF_ActiveSkillDataAsset>() && !IsA<UNF_PassiveSkillDataAsset>())) { return {}; }
	return ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, AbilityLevel, InputID, const_cast<UNF_SkillDataAsset*>(this)));
}

bool UNF_SkillDataAsset::GetBuildItemData_Implementation(FNF_BuildItemData& OutData) const
{
	OutData = FNF_BuildItemData();
	if (!HasValidSettings()) { return false; }
	FNF_BuildItemData Data;
	Data.ItemType = AbilityClass->IsChildOf(UNFGA_PassiveSkillBase::StaticClass())
		? ENF_ItemType::PassiveSkill : ENF_ItemType::ActiveSkill;
	Data.Cost = BuildCost;
	Data.AbilityClass = AbilityClass;
	Data.AbilityLevel = AbilityLevel;
	OutData = MoveTemp(Data);
	return true;
}
