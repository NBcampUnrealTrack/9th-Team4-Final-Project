#include "GamePlay/Combat/Projectiles/NF_ProjectileTrajectoryComponent.h"

#include "DrawDebugHelpers.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

UNF_ProjectileTrajectoryComponent::UNF_ProjectileTrajectoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNF_ProjectileTrajectoryComponent::StartPreview()
{
	bPreviewEnabled = true;
	bHasPredictedImpact = false;
}

void UNF_ProjectileTrajectoryComponent::StopPreview()
{
	bPreviewEnabled = false;
	bHasPredictedImpact = false;
}

bool UNF_ProjectileTrajectoryComponent::GetPredictedImpact(FHitResult& OutHit) const
{
	OutHit = bHasPredictedImpact ? PredictedImpact : FHitResult();
	return bHasPredictedImpact;
}

void UNF_ProjectileTrajectoryComponent::UpdatePreview(FVector StartLocation, FVector LaunchVelocity,
	float ProjectileRadius, float GravityZ, float FlightTime)
{
	bHasPredictedImpact = false;
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!bPreviewEnabled || !Pawn || !Pawn->IsLocallyControlled() || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	FPredictProjectilePathParams Params(FMath::Max(ProjectileRadius, 1.0f), StartLocation,
		LaunchVelocity, FMath::Max(FlightTime, 0.1f), TraceChannel, GetOwner());
	Params.SimFrequency = FMath::Clamp(SimulationFrequency, 10.0f, 60.0f);
	// PredictProjectilePath는 0을 월드 중력 사용으로 해석하므로 무중력은 극소값으로 표현한다.
	Params.OverrideGravityZ = FMath::IsNearlyZero(GravityZ) ? SMALL_NUMBER : GravityZ;
	FPredictProjectilePathResult Result;
	const bool bHit = UGameplayStatics::PredictProjectilePath(this, Params, Result);
	bHasPredictedImpact = bHit;
	PredictedImpact = Result.HitResult;
	for (int32 Index = 1; Index < Result.PathData.Num(); ++Index)
	{
		DrawDebugLine(GetWorld(), Result.PathData[Index - 1].Location, Result.PathData[Index].Location,
			PathColor, false, 0.0f, 0, FMath::Max(LineThickness, 0.1f));
	}
	if (bHit)
	{
		DrawDebugSphere(GetWorld(), Result.HitResult.ImpactPoint, FMath::Max(ImpactMarkerRadius, 1.0f),
			12, ImpactColor, false, 0.0f, 0, FMath::Max(LineThickness, 0.1f));
	}
}
