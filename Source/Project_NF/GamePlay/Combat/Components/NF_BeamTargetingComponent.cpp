#include "GamePlay/Combat/Components/NF_BeamTargetingComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "UObject/ConstructorHelpers.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

UNF_BeamTargetingComponent::UNF_BeamTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	VisualMesh = Cube.Object;
}

bool UNF_BeamTargetingComponent::CalculateBeam(AActor* Avatar, FVector& Start, FVector& End) const
{
	if (!IsValid(Avatar) || !GetWorld() || !FMath::IsFinite(Range) || Range <= 0.0f
		|| !FMath::IsFinite(HalfWidth) || HalfWidth <= 0.0f || ViewOffset.ContainsNaN()) { return false; }
	FVector Eyes;
	FRotator Aim;
	Avatar->GetActorEyesViewPoint(Eyes, Aim);
	Start = Eyes + Aim.RotateVector(ViewOffset);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NF_BeamTarget), false, Avatar);
	Params.AddIgnoredActor(GetOwner());
	FHitResult Hit;
	const FVector AimEnd = Eyes + Aim.Vector() * Range;
	End = GetWorld()->LineTraceSingleByChannel(Hit, Eyes, AimEnd, TraceChannel, Params) ? Hit.ImpactPoint : AimEnd;
	// 눈과 발사 위치가 다를 때 발사 위치 앞의 벽도 검사한다.
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, TraceChannel, Params)) { End = Hit.ImpactPoint; }
	return !Start.ContainsNaN() && !End.ContainsNaN() && FVector::DistSquared(Start, End) > 1.0;
}

void UNF_BeamTargetingComponent::ShowBeam(FVector Start, FVector End)
{
	if (GetNetMode() == NM_DedicatedServer || Start.ContainsNaN() || End.ContainsNaN()) { HideBeam(); return; }
	if (VisualNiagara)
	{
		if (BeamMesh) { BeamMesh->SetVisibility(false); }
		if (!BeamNiagara)
		{
			BeamNiagara = NewObject<UNiagaraComponent>(GetOwner());
			BeamNiagara->SetAutoActivate(false);
			BeamNiagara->SetAutoDestroy(false);
			BeamNiagara->SetAsset(VisualNiagara);
			BeamNiagara->RegisterComponent();
		}
		else if (BeamNiagara->GetAsset() != VisualNiagara)
		{
			BeamNiagara->DeactivateImmediate();
			BeamNiagara->SetAsset(VisualNiagara);
		}
		const FQuat BeamRotation = (End - Start).Rotation().Quaternion();
		const FQuat OffsetRotation = FRotator(VisualRotationOffset.Y, VisualRotationOffset.Z, VisualRotationOffset.X).Quaternion();
		BeamNiagara->SetWorldLocationAndRotation(Start, BeamRotation * OffsetRotation);
		if (!BeamStartParameter.IsNone()) { BeamNiagara->SetVariablePosition(BeamStartParameter, Start); }
		if (!BeamEndParameter.IsNone()) { BeamNiagara->SetVariablePosition(BeamEndParameter, End); }
		if (!BeamWidthParameter.IsNone()) { BeamNiagara->SetVariableFloat(BeamWidthParameter, HalfWidth * 2.0f); }
		BeamNiagara->SetVisibility(true);
		if (!BeamNiagara->IsActive()) { BeamNiagara->Activate(true); }
		return;
	}
	if (BeamNiagara) { BeamNiagara->DeactivateImmediate(); BeamNiagara->SetVisibility(false); }
	if (!VisualMesh) { HideBeam(); return; }
	if (!BeamMesh)
	{
		BeamMesh = NewObject<UStaticMeshComponent>(GetOwner());
		BeamMesh->SetMobility(EComponentMobility::Movable);
		BeamMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BeamMesh->SetCastShadow(false);
		BeamMesh->SetStaticMesh(VisualMesh);
		if (VisualMaterial) { BeamMesh->SetMaterial(0, VisualMaterial); }
		BeamMesh->RegisterComponent();
	}
	const FVector Delta = End - Start;
	BeamMesh->SetWorldTransform(FTransform(Delta.Rotation(), (Start + End) * 0.5f,
		FVector(Delta.Size() / 100.0f, HalfWidth * 0.02f, HalfWidth * 0.02f)));
	BeamMesh->SetVisibility(true);
}
void UNF_BeamTargetingComponent::HideBeam()
{
	if (BeamMesh) { BeamMesh->SetVisibility(false); }
	if (BeamNiagara) { BeamNiagara->DeactivateImmediate(); BeamNiagara->SetVisibility(false); }
}
void UNF_BeamTargetingComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (BeamMesh) { BeamMesh->DestroyComponent(); BeamMesh = nullptr; }
	if (BeamNiagara) { BeamNiagara->DestroyComponent(); BeamNiagara = nullptr; }
	Super::EndPlay(Reason);
}
