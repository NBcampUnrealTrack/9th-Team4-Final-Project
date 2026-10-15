#include "GamePlay/Combat/Components/NF_TargetQueryComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "GenericTeamAgentInterface.h"
#include "GameplayTagAssetInterface.h"

bool NF_BuildQueryShape(const FNF_AreaDamageShape& S, FCollisionShape& Out)
{
	switch (S.Shape)
	{
	case ENF_AreaDamageShape::Sphere:
		if (!FMath::IsFinite(S.Radius) || S.Radius <= 0.0f) { return false; }
		Out = FCollisionShape::MakeSphere(S.Radius); return true;
	case ENF_AreaDamageShape::Box:
		if (S.BoxHalfExtent.ContainsNaN() || S.BoxHalfExtent.GetMin() <= 0.0) { return false; }
		Out = FCollisionShape::MakeBox(S.BoxHalfExtent); return true;
	case ENF_AreaDamageShape::Capsule:
		if (!FMath::IsFinite(S.Radius) || S.Radius <= 0.0f || !FMath::IsFinite(S.CapsuleHalfHeight) || S.CapsuleHalfHeight < S.Radius) { return false; }
		Out = FCollisionShape::MakeCapsule(S.Radius, S.CapsuleHalfHeight); return true;
	case ENF_AreaDamageShape::Sector:
		if (!FMath::IsFinite(S.Radius) || S.Radius <= 0.0f || !FMath::IsFinite(S.SectorHalfHeight) || S.SectorHalfHeight <= 0.0f
			|| !FMath::IsFinite(S.SectorHalfAngle) || S.SectorHalfAngle < 0.0f || S.SectorHalfAngle > 180.0f) { return false; }
		Out = FCollisionShape::MakeBox(FVector(S.Radius, S.Radius, S.SectorHalfHeight)); return true;
	default: return false;
	}
}

namespace
{
	FGenericTeamId ResolveTeam(const AActor* Actor)
	{
		FGenericTeamId Id = FGenericTeamId::GetTeamIdentifier(Actor);
		if (Id == FGenericTeamId::NoTeam)
		{
			if (const APawn* Pawn = Cast<APawn>(Actor)) { Id = FGenericTeamId::GetTeamIdentifier(Pawn->GetController()); }
		}
		return Id;
	}
	FCollisionObjectQueryParams BuildObjects(const FNF_TargetQuerySettings& S)
	{
		FCollisionObjectQueryParams Objects;
		for (ECollisionChannel Channel : S.ObjectChannels)
		{
			if (Channel >= ECC_WorldStatic && Channel < ECC_OverlapAll_Deprecated && FCollisionObjectQueryParams::IsValidObjectQuery(Channel))
			{
				Objects.AddObjectTypesToQuery(Channel);
			}
		}
		return Objects;
	}
}

UNF_TargetQueryComponent::UNF_TargetQueryComponent() { PrimaryComponentTick.bCanEverTick = false; }
bool UNF_TargetQueryComponent::HasValidSettings() const
{
	FCollisionShape Shape;
	return Settings.MaxTargets >= 0 && NF_BuildQueryShape(Settings.Shape, Shape) && BuildObjects(Settings).IsValid();
}

bool UNF_TargetQueryComponent::PassesFilter(AActor* Target, AActor* Source, const FNF_TargetFilterSettings& Filter)
{
	if (!IsValid(Target) || Target->IsActorBeingDestroyed() || (!Filter.bIncludeSource && Target == Source)) { return false; }
	const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	if (Filter.bRequireAbilitySystem && !IsValid(ASC)) { return false; }
	FGameplayTagContainer Tags;
	if (ASC) { ASC->GetOwnedGameplayTags(Tags); }
	else if (const auto* TagOwner = Cast<IGameplayTagAssetInterface>(Target)) { TagOwner->GetOwnedGameplayTags(Tags); }
	if (!Tags.HasAll(Filter.RequiredTags) || Tags.HasAny(Filter.BlockedTags)) { return false; }
	if (Filter.Team != ENF_TargetTeamFilter::All)
	{
		const FGenericTeamId SourceTeam = ResolveTeam(Source);
		const FGenericTeamId TargetTeam = ResolveTeam(Target);
		if (SourceTeam == FGenericTeamId::NoTeam || TargetTeam == FGenericTeamId::NoTeam) { return false; }
		const ETeamAttitude::Type Attitude = FGenericTeamId::GetAttitude(SourceTeam, TargetTeam);
		if ((Filter.Team == ENF_TargetTeamFilter::Allies && Attitude != ETeamAttitude::Friendly)
			|| (Filter.Team == ENF_TargetTeamFilter::Enemies && Attitude != ETeamAttitude::Hostile)) { return false; }
	}
	return true;
}

int32 UNF_TargetQueryComponent::Query(UWorld* World, AActor* Authority, AActor* Source, FTransform Transform,
	const FNF_TargetQuerySettings& S, const FNF_AreaDamageTargetFilter& Filter, TArray<FNF_TargetQueryResult>& Out)
{
	Out.Reset();
	FCollisionShape Shape;
	const FVector Center = Transform.GetLocation();
	const FQuat Rotation = Transform.GetRotation();
	if (!World || !IsValid(Authority) || !Authority->HasAuthority() || Center.ContainsNaN() || Rotation.ContainsNaN()
		|| !Rotation.IsNormalized() || S.MaxTargets < 0 || !NF_BuildQueryShape(S.Shape, Shape)) { return 0; }
	const FCollisionObjectQueryParams Objects = BuildObjects(S);
	if (!Objects.IsValid()) { return 0; }
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NF_TargetQuery), false);
	if (Authority != Source || !S.Filter.bIncludeSource) { Params.AddIgnoredActor(Authority); }
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, Center, Rotation, Objects, Shape, Params);
	TSet<AActor*> Actors;
	TSet<UAbilitySystemComponent*> AbilitySystems;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();
		UPrimitiveComponent* Component = Overlap.GetComponent();
		if (!IsValid(Authority) || Authority->IsActorBeingDestroyed()) { break; }
		if (!IsValid(Component) || Actors.Contains(Target) || !PassesFilter(Target, Source, S.Filter)) { continue; }
		const FVector Point = Component->Bounds.Origin;
		if (Point.ContainsNaN()) { continue; }
		if (S.Shape.Shape == ENF_AreaDamageShape::Sector)
		{
			const FVector Local = Rotation.UnrotateVector(Point - Center);
			const double Distance = Local.Size2D();
			if (FMath::Abs(Local.Z) > S.Shape.SectorHalfHeight || Distance > S.Shape.Radius
				|| (Distance > UE_SMALL_NUMBER && Local.X / Distance < FMath::Cos(FMath::DegreesToRadians(S.Shape.SectorHalfAngle)))) { continue; }
		}
		if (Filter.IsBound() && !Filter.Execute(Target)) { continue; }
		if (!IsValid(Target) || !IsValid(Component) || !IsValid(Authority)) { continue; }
		UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
		if (S.bDeduplicateAbilitySystems && ASC && AbilitySystems.Contains(ASC)) { continue; }
		FHitResult Obstacle;
		if (S.bRequireLineOfSight && World->LineTraceSingleByChannel(Obstacle, Center, Point, S.TraceChannel, Params) && Obstacle.GetActor() != Target) { continue; }
		Actors.Add(Target);
		if (ASC) { AbilitySystems.Add(ASC); }
		FNF_TargetQueryResult& Result = Out.AddDefaulted_GetRef();
		Result.Target = Target;
		Result.Distance = FVector::Distance(Center, Point);
		Result.Hit = FHitResult(Target, Component, Point, (Center - Point).GetSafeNormal());
		Result.Hit.TraceStart = Center; Result.Hit.TraceEnd = Point;
	}
	Out.RemoveAll([](const FNF_TargetQueryResult& Result) { return !IsValid(Result.Target); });
	if (S.bSortByDistance)
	{
		Out.Sort([](const FNF_TargetQueryResult& A, const FNF_TargetQueryResult& B)
		{
			return A.Distance != B.Distance ? A.Distance < B.Distance : A.Target->GetUniqueID() < B.Target->GetUniqueID();
		});
	}
	if (S.MaxTargets > 0 && Out.Num() > S.MaxTargets) { Out.SetNum(S.MaxTargets); }
	return Out.Num();
}

int32 UNF_TargetQueryComponent::FindTargets(FTransform Transform, AActor* Source, TArray<FNF_TargetQueryResult>& Out)
{
	return FindTargetsWithShape(Transform, Settings.Shape, Source, Out);
}
int32 UNF_TargetQueryComponent::FindTargetsWithShape(FTransform Transform, const FNF_AreaDamageShape& Shape,
	AActor* Source, TArray<FNF_TargetQueryResult>& Out)
{
	const FNF_AreaDamageTargetFilter Filter;
	FNF_TargetQuerySettings QuerySettings = Settings;
	QuerySettings.Shape = Shape;
	const int32 Count = Query(GetWorld(), GetOwner(), Source, Transform, QuerySettings, Filter, Out);
	if (GetOwner() && GetOwner()->HasAuthority()) { OnTargetsFound.Broadcast(Out); }
	return Count;
}
