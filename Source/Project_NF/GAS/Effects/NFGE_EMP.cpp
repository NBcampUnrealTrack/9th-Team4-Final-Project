#include "GAS/Effects/NFGE_EMP.h"
#include "GAS/Tags/NFTag_Skill.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UNFGE_EMP::UNFGE_EMP()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(3.0f));
	UTargetTagsGameplayEffectComponent* Tags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("EMPGrantedTags"));
	GEComponents.Add(Tags);
	FInheritedTagContainer Changes;
	Changes.AddTag(NFTag_Skill::State_Debuff_EMP);
	Tags->SetAndApplyTargetTagChanges(Changes);
}
