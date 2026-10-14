#include "GamePlay/Combat/Components/NF_GroundTargetingComponent.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"

UNF_GroundTargetingComponent::UNF_GroundTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UNF_GroundTargetingComponent::FindGroundTarget(AActor* SourceAvatar, float TargetRange,
	float GroundSearchDepth, FHitResult& OutGroundHit, FVector& OutDirection) const
{
	return CalculateGroundTarget(SourceAvatar, TargetRange, GroundSearchDepth, OutGroundHit, OutDirection);
}

bool UNF_GroundTargetingComponent::CalculateGroundTarget(AActor* SourceAvatar, float TargetRange,
	float GroundSearchDepth, FHitResult& OutGroundHit, FVector& OutDirection)
{
	OutGroundHit = FHitResult();
	OutDirection = FVector::ZeroVector;
	if (!IsValid(SourceAvatar) || !SourceAvatar->GetWorld())
	{
		return false;
	}
	FVector Eyes;
	FRotator Aim;
	SourceAvatar->GetActorEyesViewPoint(Eyes, Aim);
	const float Range = FMath::Max(TargetRange, 100.0f);
	const FVector End = Eyes + Aim.Vector() * Range;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(NF_GroundTarget), false, SourceAvatar);
	FHitResult Hit;
	if (SourceAvatar->GetWorld()->LineTraceSingleByChannel(Hit, Eyes, End, ECC_Visibility, Query))
	{
		if (Hit.ImpactNormal.Z < 0.3f)
		{
			return false;
		}
	}
	else if (!SourceAvatar->GetWorld()->LineTraceSingleByChannel(Hit, End + FVector::UpVector * 1000.0f,
		End - FVector::UpVector * FMath::Max(GroundSearchDepth, 100.0f), ECC_Visibility, Query)
		|| Hit.ImpactNormal.Z < 0.3f)
	{
		return false;
	}
	if (FVector::DistSquared2D(SourceAvatar->GetActorLocation(), Hit.ImpactPoint) > FMath::Square(Range))
	{
		return false;
	}
	OutGroundHit = Hit;
	OutDirection = FVector(Aim.Vector().X, Aim.Vector().Y, 0.0f).GetSafeNormal();
	if (OutDirection.IsNearlyZero())
	{
		OutDirection = FVector::ForwardVector;
	}
	return true;
}
