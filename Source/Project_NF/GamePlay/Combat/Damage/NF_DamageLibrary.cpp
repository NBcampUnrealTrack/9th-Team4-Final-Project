#include "GamePlay/Combat/Damage/NF_DamageLibrary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "GamePlay/Combat/Components/NF_ApplyEffectComponent.h"

bool UNF_DamageLibrary::SubmitDamageSpec(AActor* AuthorityActor, AActor* Target,
	const FGameplayEffectSpecHandle& DamageSpec, FVector DamageOrigin, const FHitResult& TargetHit)
{
	return UNF_ApplyEffectComponent::SubmitSpec(AuthorityActor, Target, DamageSpec, DamageOrigin, TargetHit);
}
