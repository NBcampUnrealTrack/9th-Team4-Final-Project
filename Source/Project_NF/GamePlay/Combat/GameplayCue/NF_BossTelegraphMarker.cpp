#include "GamePlay/Combat/GameplayCue/NF_BossTelegraphMarker.h"

#include "Components/DecalComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

ANF_BossTelegraphMarker::ANF_BossTelegraphMarker()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = false;

	TelegraphDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("TelegraphDecal"));
	SetRootComponent(TelegraphDecal);
	TelegraphDecal->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	TelegraphDecal->SetVisibility(false);
	TelegraphDecal->SetHiddenInGame(true);
}

void ANF_BossTelegraphMarker::InitializeMarker(const FVector& Location, const float Radius,
	const float InProgressDuration, const float InLifetime, UMaterialInterface* Material, const FName InProgressParameterName,
	const float ProjectionDepth, const float HeightOffset)
{
	MarkerLocation = Location;
	MarkerRadius = FMath::Max(Radius, 0.0f);
	ProgressDuration = FMath::Max(InProgressDuration, 0.01f);
	MarkerLifetime = FMath::Max(InLifetime, 0.01f);
	MarkerMaterial = Material;
	ProgressParameterName = InProgressParameterName;
	MarkerProjectionDepth = FMath::Max(ProjectionDepth, 0.0f);
	MarkerHeightOffset = HeightOffset;
}

void ANF_BossTelegraphMarker::BeginPlay()
{
	Super::BeginPlay();
	if (MarkerRadius <= 0.0f || !IsValid(MarkerMaterial))
	{
		Destroy();
		return;
	}

	TelegraphDecal->SetDecalMaterial(MarkerMaterial);
	DynamicMaterial = TelegraphDecal->CreateDynamicMaterialInstance();
	if (IsValid(DynamicMaterial) && !ProgressParameterName.IsNone())
	{
		DynamicMaterial->SetScalarParameterValue(ProgressParameterName, 0.0f);
	}
	// M_StairBombTelegraph's circular SphereMask radius is 0.48 around UV center 0.5.
	// Expand the projected decal bounds so the visible circle reaches the gameplay radius.
	constexpr float MaterialMaskRadius = 0.48f;
	constexpr float UvCenterToEdge = 0.5f;
	const float VisualRadius = MarkerRadius * (UvCenterToEdge / MaterialMaskRadius);
	TelegraphDecal->DecalSize = FVector(MarkerProjectionDepth, VisualRadius, VisualRadius);
	TelegraphDecal->SetWorldLocationAndRotation(
		MarkerLocation + FVector::UpVector * MarkerHeightOffset,
		FRotator(-90.0f, 0.0f, 0.0f));
	TelegraphDecal->SetHiddenInGame(false);
	TelegraphDecal->SetVisibility(true);
	SetActorTickEnabled(true);
}

void ANF_BossTelegraphMarker::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ElapsedTime += DeltaSeconds;
	const float Progress = FMath::Clamp(ElapsedTime / ProgressDuration, 0.0f, 1.0f);
	if (IsValid(DynamicMaterial) && !ProgressParameterName.IsNone())
	{
		DynamicMaterial->SetScalarParameterValue(ProgressParameterName, Progress);
	}
	if (ElapsedTime >= MarkerLifetime)
	{
		Destroy();
	}
}
