#include "GamePlay/Combat/Damage/NF_DamageLibrary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"

bool UNF_DamageLibrary::SubmitDamageSpec(AActor* AuthorityActor, AActor* Target,
	const FGameplayEffectSpecHandle& DamageSpec, FVector DamageOrigin, const FHitResult& TargetHit)
{
	if (!IsValid(AuthorityActor) || !AuthorityActor->HasAuthority()
		|| !IsValid(Target) || !Target->HasAuthority() || !DamageSpec.IsValid()
		|| !DamageSpec.Data->GetContext().IsValid())
	{
		return false;
	}
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!IsValid(TargetASC))
	{
		return false;
	}

	FGameplayEffectSpec TargetSpec(*DamageSpec.Data.Get());
	FGameplayEffectContextHandle Context = TargetSpec.GetContext().Duplicate();
	Context.AddOrigin(DamageOrigin);
	Context.AddHitResult(TargetHit, true);
	// 대상 정보만 추가한다. 발사/시전 시 저장된 시전자 태그는 유지한다.
	TargetSpec.SetContext(Context, true);
	TargetASC->ApplyGameplayEffectSpecToSelf(TargetSpec);
	return true;
}
