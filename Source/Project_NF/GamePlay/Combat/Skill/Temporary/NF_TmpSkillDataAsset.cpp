#include "GamePlay/Combat/Skill/Temporary/NF_TmpSkillDataAsset.h"
#include "GamePlay/Combat/Skill/Temporary/NF_TmpActiveSkillBase.h"
#include "GamePlay/Combat/Skill/Temporary/NF_TmpPassiveSkillBase.h"
#include "AbilitySystemComponent.h"
#include "GAS/Tags/NFTag_Player.h"
#include "GAS/Tags/NFTag_Skill.h"

UNF_TmpActiveSkillDataAsset::UNF_TmpActiveSkillDataAsset()
{
	SkillType = ENF_TmpSkillType::Active;
	FNF_TmpSkillStateRule Death; Death.StateTag = NFTag_Player::Player_State_Death; Death.bForceCancel = true;
	FNF_TmpSkillStateRule EMP; EMP.StateTag = NFTag_Skill::State_Debuff_EMP; EMP.bForceCancel = true;
	StateRules = { Death, EMP };
}

UNF_TmpPassiveSkillDataAsset::UNF_TmpPassiveSkillDataAsset()
{
	SkillType = ENF_TmpSkillType::Passive;
	FNF_TmpSkillStateRule Death; Death.StateTag = NFTag_Player::Player_State_Death; Death.bForceCancel = true;
	StateRules = { Death };
}

bool UNF_TmpSkillDataAsset::HasValidSettings() const
{
	if (!AbilityClass || AbilityClass->HasAnyClassFlags(CLASS_Abstract) || AbilityLevel < 1 || Cost < 0) { return false; }
	if (SkillType == ENF_TmpSkillType::Active)
	{
		return IsA<UNF_TmpActiveSkillDataAsset>() && AbilityClass->IsChildOf(UNF_TmpActiveSkillBase::StaticClass());
	}
	return SkillType == ENF_TmpSkillType::Passive && IsA<UNF_TmpPassiveSkillDataAsset>()
		&& AbilityClass->IsChildOf(UNF_TmpPassiveSkillBase::StaticClass());
}

FGameplayAbilitySpecHandle UNF_TmpSkillDataAsset::GrantTo(UAbilitySystemComponent* ASC) const
{
	if (!IsValid(ASC) || !ASC->IsOwnerActorAuthoritative() || !HasValidSettings()) { return {}; }
	return ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, AbilityLevel, INDEX_NONE,
		const_cast<UNF_TmpSkillDataAsset*>(this)));
}
