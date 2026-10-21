#include "GAS/Effects/NFGE_DoubleJump.h"
#include "GAS/Tags/NFTag_Skill.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UNFGE_DoubleJump::UNFGE_DoubleJump()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	auto* Tags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("DoubleJumpGrantedTags"));
	GEComponents.Add(Tags);
	FInheritedTagContainer Changes;
	Changes.AddTag(NFTag_Skill::State_DoubleJumpEnabled);
	Tags->SetAndApplyTargetTagChanges(Changes);
}
