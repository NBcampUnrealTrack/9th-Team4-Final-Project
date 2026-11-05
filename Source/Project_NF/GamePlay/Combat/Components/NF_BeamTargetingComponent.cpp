#include "GamePlay/Combat/Components/NF_BeamTargetingComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "UObject/ConstructorHelpers.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Components/DecalComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

UNF_BeamTargetingComponent::UNF_BeamTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	VisualMesh = Cube.Object;
}

bool UNF_BeamTargetingComponent::CalculateBeam(AActor* Avatar, FVector& Start, FVector& End, FHitResult* OutHit) const
{
	if (OutHit) { *OutHit = FHitResult(); }
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
	const bool bAimHit = GetWorld()->LineTraceSingleByChannel(Hit, Eyes, AimEnd, TraceChannel, Params);
	End = bAimHit ? Hit.ImpactPoint : AimEnd;
	const FHitResult AimHit = Hit;
	// 눈과 발사 위치가 다를 때 발사 위치 앞의 벽도 검사한다.
	// ViewOffset으로 시작점이 이동해도 실제 빔 길이는 Range 이내로 제한한다.
	End = Start + (End - Start).GetClampedToMaxSize(Range);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, TraceChannel, Params))
	{
		End = Hit.ImpactPoint;
		if (OutHit) { *OutHit = Hit; }
	}
	else if (OutHit && bAimHit && End.Equals(AimHit.ImpactPoint, KINDA_SMALL_NUMBER))
	{
		// 표면에서 정확히 끝나는 두 번째 트레이스가 누락되면 원래 조준 충돌을 사용한다.
		*OutHit = AimHit;
	}
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
		const FVector Delta = End - Start;
		const float BeamLength = Delta.Size();
		const float ReferenceLength = FMath::IsFinite(NiagaraReferenceLength) && NiagaraReferenceLength > 0.0f
			? NiagaraReferenceLength : 330.0f;
		const FQuat BeamRotation = Delta.Rotation().Quaternion();
		const FQuat OffsetRotation = FRotator(VisualRotationOffset.Y, VisualRotationOffset.Z, VisualRotationOffset.X).Quaternion();
		const FVector VisualOrigin = bNiagaraOriginAtCenter ? (Start + End) * 0.5f : Start;
		BeamNiagara->SetWorldLocationAndRotation(VisualOrigin, BeamRotation * OffsetRotation);
		if (!BeamScaleParameter.IsNone()) { BeamNiagara->SetVariableVec3(BeamScaleParameter, FVector(1.0f, 1.0f, BeamLength / ReferenceLength)); }
		if (!BeamLengthParameter.IsNone()) { BeamNiagara->SetVariableFloat(BeamLengthParameter, BeamLength); }
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
void UNF_BeamTargetingComponent::UpdateImpactDecal(bool bSurfaceHit, FVector Location, FVector Normal)
{
	if (GetNetMode() == NM_DedicatedServer || !GetWorld()) { return; }
	if (!bSurfaceHit || Location.ContainsNaN() || Normal.ContainsNaN() || Normal.IsNearlyZero())
	{
		bWasHittingSurface = false;
		return;
	}
	if (ImpactDecalSize.ContainsNaN() || ImpactDecalSize.GetMin() <= 0.0f
		|| ImpactDecalRotationRange.ContainsNaN() || ImpactDecalScaleRange.ContainsNaN() || ImpactDecalScaleRange.GetMin() <= 0.0f
		|| !FMath::IsFinite(ImpactDecalLifetime) || ImpactDecalLifetime <= 0.0f
		|| !FMath::IsFinite(ImpactDecalFadeInDuration) || ImpactDecalFadeInDuration < 0.0f
		|| !FMath::IsFinite(ImpactDecalFadeDuration) || ImpactDecalFadeDuration < 0.0f
		|| !FMath::IsFinite(ImpactDecalInterval) || ImpactDecalInterval <= 0.0f
		|| !FMath::IsFinite(ImpactDecalSpacing) || ImpactDecalSpacing <= 0.0f || MaxImpactDecals <= 0) { return; }
	const double Now = GetWorld()->GetTimeSeconds();
	if (LastDecalTime >= 0.0 && Now - LastDecalTime < ImpactDecalInterval) { return; }
	const FVector SurfaceNormal = Normal.GetSafeNormal();
	const bool bMoved = FVector::DistSquared(Location, LastDecalLocation) >= FMath::Square(ImpactDecalSpacing);
	const bool bChangedSurface = FVector::DotProduct(SurfaceNormal, LastDecalNormal) < 0.95f;
	if (bWasHittingSurface && !bMoved && !bChangedSurface && Now - LastDecalTime < ImpactDecalLifetime) { return; }
	const auto IsDecalMaterial = [](UMaterialInterface* Material)
	{
		return IsValid(Material) && Material->GetMaterial() && Material->GetMaterial()->MaterialDomain == MD_DeferredDecal;
	};
	UMaterialInterface* Material = ImpactDecalMaterial.Get();
	if (!IsDecalMaterial(Material)) { return; }
	const bool bUseVariants = ImpactDecalVariantCount > 1 && !ImpactDecalVariantParameter.IsNone();
	const int32 VariantIndex = bUseVariants ? FMath::RandRange(0, ImpactDecalVariantCount - 1) : 0;
	// 외형용 랜덤 값은 클라이언트별로 선택한다. 공격 판정과 복제된 충돌 위치는 유지한다.
	const float Angle = FMath::FRandRange(FMath::Min(ImpactDecalRotationRange.X, ImpactDecalRotationRange.Y),
		FMath::Max(ImpactDecalRotationRange.X, ImpactDecalRotationRange.Y));
	const float Scale = FMath::FRandRange(FMath::Min(ImpactDecalScaleRange.X, ImpactDecalScaleRange.Y),
		FMath::Max(ImpactDecalScaleRange.X, ImpactDecalScaleRange.Y));
	const FVector DecalSize(ImpactDecalSize.X, ImpactDecalSize.Y * Scale, ImpactDecalSize.Z * Scale);
	// 로컬 X(투영 축) 주위로 돌려 표면에 붙는 방향은 유지한다.
	const FQuat Rotation = SurfaceNormal.Rotation().Quaternion() * FQuat(FVector::ForwardVector, FMath::DegreesToRadians(Angle));
	ImpactDecals.RemoveAll([](const TWeakObjectPtr<UDecalComponent>& Decal) { return !Decal.IsValid(); });
	while (ImpactDecals.Num() >= MaxImpactDecals)
	{
		if (UDecalComponent* Oldest = ImpactDecals[0].Get()) { Oldest->DestroyComponent(); }
		ImpactDecals.RemoveAt(0);
	}
	// 월드에 독립적으로 생성해 스킬 종료 후에도 수명이 끝날 때까지 흔적을 유지한다.
	UDecalComponent* Decal = UGameplayStatics::SpawnDecalAtLocation(GetWorld(), Material,
		DecalSize, Location, Rotation.Rotator(), ImpactDecalLifetime);
	if (!Decal) { return; }
	if (bUseVariants)
	{
		// 데칼마다 독립된 MID를 사용해 이미 생성한 흔적의 이미지가 함께 바뀌지 않게 한다.
		UMaterialInstanceDynamic* Instance = Decal->CreateDynamicMaterialInstance();
		if (!Instance) { Decal->DestroyComponent(); return; }
		Instance->SetScalarParameterValue(ImpactDecalVariantParameter, static_cast<float>(VariantIndex));
	}
	const float FadeDuration = FMath::Min(ImpactDecalFadeDuration, ImpactDecalLifetime);
	const float FadeInDuration = FMath::Min(ImpactDecalFadeInDuration, ImpactDecalLifetime);
	if (FadeInDuration > 0.0f) { Decal->SetFadeIn(0.0f, FadeInDuration); }
	if (FadeDuration > 0.0f) { Decal->SetFadeOut(ImpactDecalLifetime - FadeDuration, FadeDuration, false); }
	ImpactDecals.Add(Decal);
	LastDecalLocation = Location;
	LastDecalNormal = SurfaceNormal;
	LastDecalTime = Now;
	bWasHittingSurface = true;
}

void UNF_BeamTargetingComponent::HideBeam()
{
	bWasHittingSurface = false;
	if (BeamMesh) { BeamMesh->SetVisibility(false); }
	if (BeamNiagara) { BeamNiagara->DeactivateImmediate(); BeamNiagara->SetVisibility(false); }
}
void UNF_BeamTargetingComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (BeamMesh) { BeamMesh->DestroyComponent(); BeamMesh = nullptr; }
	if (BeamNiagara) { BeamNiagara->DestroyComponent(); BeamNiagara = nullptr; }
	Super::EndPlay(Reason);
}
