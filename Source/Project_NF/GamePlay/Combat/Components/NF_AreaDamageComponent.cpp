#include "GamePlay/Combat/Components/NF_AreaDamageComponent.h"
#include "GamePlay/Combat/Components/NF_TargetQueryComponent.h"
#include "GamePlay/Combat/Damage/NF_DamageLibrary.h"
#include "Engine/World.h"

UNF_AreaDamageComponent::UNF_AreaDamageComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	TargetObjectChannels = { ECC_Pawn, ECC_WorldDynamic, ECC_PhysicsBody };
}
bool UNF_AreaDamageComponent::HasValidSettings() const
{
	FCollisionShape Shape;
	return !TargetObjectChannels.IsEmpty() && NF_BuildQueryShape(DefaultShape, Shape);
}
void UNF_AreaDamageComponent::SetOcclusionPolicy(bool bRequireVisibility, ECollisionChannel Channel)
{
	bRequireLineOfSight = bRequireVisibility; OcclusionTraceChannel = Channel;
}
int32 UNF_AreaDamageComponent::SubmitSphereDamage(FVector Center, float Radius, const FGameplayEffectSpecHandle& Spec,
	AActor* Source, const FNF_AreaDamageTargetFilter& Filter)
{
	FNF_AreaDamageShape Shape; Shape.Radius = Radius;
	return SubmitAreaDamage(FTransform(Center), Shape, Spec, Source, Filter);
}
int32 UNF_AreaDamageComponent::SubmitConfiguredDamage(FTransform Transform, const FGameplayEffectSpecHandle& Spec,
	AActor* Source, const FNF_AreaDamageTargetFilter& Filter)
{
	return SubmitAreaDamage(Transform, DefaultShape, Spec, Source, Filter);
}
int32 UNF_AreaDamageComponent::FindAreaTargets(FTransform Transform, const FNF_AreaDamageShape& Shape,
	AActor* Source, const FNF_AreaDamageTargetFilter& Filter, TArray<FNF_TargetQueryResult>& Out) const
{
	FNF_TargetQuerySettings Settings;
	Settings.Shape = Shape; Settings.ObjectChannels = TargetObjectChannels;
	Settings.Filter.bIncludeSource = bAffectSource; Settings.Filter.bRequireAbilitySystem = true;
	Settings.Filter.RequiredTags = RequiredTargetTags; Settings.Filter.BlockedTags = BlockedTargetTags;
	Settings.bRequireLineOfSight = bRequireLineOfSight; Settings.TraceChannel = OcclusionTraceChannel;
	Settings.bDeduplicateAbilitySystems = true;
	return UNF_TargetQueryComponent::Query(GetWorld(), GetOwner(), Source, Transform, Settings, Filter, Out);
}
int32 UNF_AreaDamageComponent::SubmitAreaDamage(FTransform Transform, const FNF_AreaDamageShape& Shape,
	const FGameplayEffectSpecHandle& Spec, AActor* Source, const FNF_AreaDamageTargetFilter& Filter)
{
	if (!Spec.IsValid()) { return 0; }
	TArray<FNF_TargetQueryResult> Targets;
	FindAreaTargets(Transform, Shape, Source, Filter, Targets);
	int32 Submitted = 0;
	for (const FNF_TargetQueryResult& Target : Targets)
	{
		Submitted += UNF_DamageLibrary::SubmitDamageSpec(GetOwner(), Target.Target, Spec, Transform.GetLocation(), Target.Hit) ? 1 : 0;
	}
	return Submitted;
}
