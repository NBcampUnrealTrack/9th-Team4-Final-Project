#include "GamePlay/Combat/Components/NF_AreaIndicatorComponent.h"

#include "Components/DecalComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"

UNF_AreaIndicatorComponent::UNF_AreaIndicatorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UNF_AreaIndicatorComponent::ShowArea(FVector Location, float Radius, FVector SurfaceNormal)
{
	if (GetNetMode() == NM_DedicatedServer || Radius <= 0.0f)
	{
		HideArea();
		return;
	}
	AreaLocation = Location;
	AreaRadius = Radius;
	AreaNormal = SurfaceNormal.GetSafeNormal();
	if (AreaNormal.IsNearlyZero())
	{
		AreaNormal = FVector::UpVector;
	}
	bAreaVisible = true;
	SetComponentTickEnabled(!DecalMaterial);
	UpdateDecal();
}

void UNF_AreaIndicatorComponent::HideArea()
{
	bAreaVisible = false;
	SetComponentTickEnabled(false);
	if (AreaDecal)
	{
		AreaDecal->SetVisibility(false);
	}
}

void UNF_AreaIndicatorComponent::UpdateDecal()
{
	if (!DecalMaterial)
	{
		if (AreaDecal)
		{
			AreaDecal->SetVisibility(false);
		}
		return;
	}
	if (!AreaDecal)
	{
		AreaDecal = NewObject<UDecalComponent>(GetOwner());
		AreaDecal->RegisterComponent();
		AreaDecal->SetFadeScreenSize(0.0f);
	}
	if (AreaDecal->GetDecalMaterial() != DecalMaterial)
	{
		AreaDecal->SetDecalMaterial(DecalMaterial);
	}
	const FVector DecalSize(FMath::Max(ProjectionDepth, 1.0f), AreaRadius, AreaRadius);
	if (!AreaDecal->DecalSize.Equals(DecalSize))
	{
		AreaDecal->DecalSize = DecalSize;
		AreaDecal->MarkRenderStateDirty();
	}
	AreaDecal->SetWorldLocationAndRotation(AreaLocation + AreaNormal * SurfaceOffset, (-AreaNormal).Rotation());
	AreaDecal->SetVisibility(bAreaVisible);
}

void UNF_AreaIndicatorComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (bAreaVisible && GetNetMode() != NM_DedicatedServer)
	{
		DrawDebugArea();
	}
}

void UNF_AreaIndicatorComponent::DrawDebugArea() const
{
	FVector AxisX;
	FVector AxisY;
	AreaNormal.FindBestAxisVectors(AxisX, AxisY);
	AxisY = FVector::CrossProduct(AreaNormal, AxisX).GetSafeNormal();
	const FVector Center = AreaLocation + AreaNormal * FMath::Max(SurfaceOffset, 0.0f);
	const int32 Segments = FMath::Clamp(CircleSegments, 12, 128);
	TArray<FVector> Vertices;
	TArray<int32> Indices;
	Vertices.Reserve(Segments + 1);
	Indices.Reserve(Segments * 3);
	Vertices.Add(Center);
	for (int32 Index = 0; Index < Segments; ++Index)
	{
		const float Angle = 2.0f * PI * Index / Segments;
		Vertices.Add(Center + (AxisX * FMath::Cos(Angle) + AxisY * FMath::Sin(Angle)) * AreaRadius);
	}
	for (int32 Index = 0; Index < Segments; ++Index)
	{
		const int32 Current = Index + 1;
		const int32 Next = (Index + 1) % Segments + 1;
		Indices.Add(0);
		Indices.Add(Current);
		Indices.Add(Next);
		DrawDebugLine(GetWorld(), Vertices[Current], Vertices[Next], BorderColor,
			false, 0.0f, 0, FMath::Max(BorderThickness, 0.1f));
	}
	if (bDrawFilledArea)
	{
		DrawDebugMesh(GetWorld(), Vertices, Indices, AreaColor, false, 0.0f);
	}
}

void UNF_AreaIndicatorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	HideArea();
	if (AreaDecal)
	{
		AreaDecal->DestroyComponent();
		AreaDecal = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}
