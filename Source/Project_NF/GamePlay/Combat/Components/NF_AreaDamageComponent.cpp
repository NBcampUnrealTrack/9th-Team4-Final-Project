#include "GamePlay/Combat/Components/NF_AreaDamageComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GamePlay/Combat/Damage/NF_DamageLibrary.h"

UNF_AreaDamageComponent::UNF_AreaDamageComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	TargetObjectChannels = { ECC_Pawn, ECC_WorldDynamic, ECC_PhysicsBody };
}

void UNF_AreaDamageComponent::SetOcclusionPolicy(bool bRequireVisibility, ECollisionChannel Channel)
{
	bRequireLineOfSight = bRequireVisibility;
	OcclusionTraceChannel = Channel;
}

int32 UNF_AreaDamageComponent::SubmitSphereDamage(FVector Center, float Radius,
	const FGameplayEffectSpecHandle& DamageSpec, AActor* SourceAvatar,
	const FNF_AreaDamageTargetFilter& TargetFilter)
{
	AActor* AuthorityActor = GetOwner();
	if (!IsValid(AuthorityActor) || !AuthorityActor->HasAuthority() || !GetWorld()
		|| !DamageSpec.IsValid() || !FMath::IsFinite(Radius) || Radius <= 0.0f
		|| TargetObjectChannels.IsEmpty())
	{
		return 0;
	}

	FCollisionObjectQueryParams ObjectParams;
	for (ECollisionChannel Channel : TargetObjectChannels)
	{
		if (Channel >= ECC_WorldStatic && Channel < ECC_OverlapAll_Deprecated
			&& FCollisionObjectQueryParams::IsValidObjectQuery(Channel))
		{
			ObjectParams.AddObjectTypesToQuery(Channel);
		}
	}
	if (!ObjectParams.IsValid())
	{
		return 0;
	}
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NF_AreaDamage), false, AuthorityActor);
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, ObjectParams,
		FCollisionShape::MakeSphere(Radius), QueryParams);

	TSet<UAbilitySystemComponent*> SubmittedComponents;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();
		if (!IsValid(Target) || (!bAffectSource && Target == SourceAvatar)
			|| (TargetFilter.IsBound() && !TargetFilter.Execute(Target)))
		{
			continue;
		}
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
		if (!IsValid(TargetASC) || SubmittedComponents.Contains(TargetASC)
			|| !TargetASC->HasAllMatchingGameplayTags(RequiredTargetTags)
			|| TargetASC->HasAnyMatchingGameplayTags(BlockedTargetTags))
		{
			continue;
		}
		const FVector TargetLocation = Target->GetActorLocation();
		FHitResult OcclusionHit;
		if (bRequireLineOfSight && GetWorld()->LineTraceSingleByChannel(OcclusionHit,
			Center, TargetLocation, OcclusionTraceChannel, QueryParams) && OcclusionHit.GetActor() != Target)
		{
			continue;
		}
		FHitResult Hit(Target, Overlap.GetComponent(), TargetLocation, (Center - TargetLocation).GetSafeNormal());
		Hit.TraceStart = Center;
		Hit.TraceEnd = TargetLocation;
		// GE 적용 중 재진입/Actor 제거가 발생해도 같은 호출에서 ASC를 중복 처리하지 않는다.
		SubmittedComponents.Add(TargetASC);
		if (!UNF_DamageLibrary::SubmitDamageSpec(AuthorityActor, Target, DamageSpec, Center, Hit))
		{
			SubmittedComponents.Remove(TargetASC);
		}
	}
	return SubmittedComponents.Num();
}
