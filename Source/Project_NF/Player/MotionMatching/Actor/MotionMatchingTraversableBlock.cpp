#include "MotionMatchingTraversableBlock.h"

#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h" // [추가] D-1
#include "Engine/StaticMesh.h" // [추가] D-1
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"

AMotionMatchingTraversableBlock::AMotionMatchingTraversableBlock()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}


void AMotionMatchingTraversableBlock::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	RefreshLedges();
}


void AMotionMatchingTraversableBlock::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	RefreshLedges();
}


void AMotionMatchingTraversableBlock::RefreshLedges()
{
	if (bIsTraversableCollisionAutoSet == true)
	{
		ApplyTraversableCollision();
	}

	CacheObstacleCenter();
	CollectLedges();

	// [추가] D-1: 스플라인을 메시 모서리에 맞춘 뒤 반대편 턱을 짝짓는다
	if (bIsLedgeAutoFitted == true)
	{
		FitLedgesToMeshBounds();
	}

	PairLedges();
}


// [추가] D-1: 충돌 있는 첫 번째 StaticMesh의 경계 상자 윗면 네 변에 스플라인 4개를 직선으로 맞춘다
void AMotionMatchingTraversableBlock::FitLedgesToMeshBounds()
{
	if (LedgeSplines.Num() < 4) return;

	UStaticMeshComponent* MeshComponent = nullptr;
	TArray<UStaticMeshComponent*> MeshComponents;
	GetComponents<UStaticMeshComponent>(MeshComponents);
	for (UStaticMeshComponent* CandidateComponent : MeshComponents)
	{
		if (IsValid(CandidateComponent) == false) continue;
		if (CandidateComponent->GetStaticMesh() == nullptr) continue;
		if (CandidateComponent->GetCollisionEnabled() == ECollisionEnabled::NoCollision) continue;

		MeshComponent = CandidateComponent;
		break;
	}

	if (MeshComponent == nullptr) return;

	// 메시 자체 좌표의 경계 상자 → 컴포넌트 Transform(액터 스케일·메시 스케일·회전 포함)으로 월드 모서리 계산
	const FBox LocalBox = MeshComponent->GetStaticMesh()->GetBoundingBox();
	const FTransform MeshTransform = MeshComponent->GetComponentTransform();
	const float TopZ = LocalBox.Max.Z;

	TArray<FVector> TopCorners;
	TopCorners.Add(MeshTransform.TransformPosition(FVector(LocalBox.Min.X, LocalBox.Min.Y, TopZ)));
	TopCorners.Add(MeshTransform.TransformPosition(FVector(LocalBox.Max.X, LocalBox.Min.Y, TopZ)));
	TopCorners.Add(MeshTransform.TransformPosition(FVector(LocalBox.Max.X, LocalBox.Max.Y, TopZ)));
	TopCorners.Add(MeshTransform.TransformPosition(FVector(LocalBox.Min.X, LocalBox.Max.Y, TopZ)));

	for (int32 EdgeIndex = 0; EdgeIndex < 4; ++EdgeIndex)
	{
		USplineComponent* Ledge = LedgeSplines[EdgeIndex];
		if (IsValid(Ledge) == false) continue;

		const TArray<FVector> EdgePoints = { TopCorners[EdgeIndex], TopCorners[(EdgeIndex + 1) % 4] };
		Ledge->SetSplinePoints(EdgePoints, ESplineCoordinateSpace::World, false);
		Ledge->SetSplinePointType(0, ESplinePointType::Linear, false);
		Ledge->SetSplinePointType(1, ESplinePointType::Linear, false);
		Ledge->UpdateSpline();
	}
}


// 충돌 있는 메쉬들의 바운드 중심을 액터 기준 로컬 좌표로 저장 (Ledge 노멀이 바깥을 향하게 할 때 기준점)
void AMotionMatchingTraversableBlock::CacheObstacleCenter()
{
	FBox Bounds(ForceInit);

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	GetComponents<UPrimitiveComponent>(PrimitiveComponents);

	for (const UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (IsValid(PrimitiveComponent) == false) continue;
		if (PrimitiveComponent->IsA<USplineComponent>() == true) continue;
		if (PrimitiveComponent->IsCollisionEnabled() == false) continue;

		Bounds += PrimitiveComponent->Bounds.GetBox();
	}

	FVector WorldCenter = GetActorLocation();
	if (Bounds.IsValid == 1)
	{
		WorldCenter = Bounds.GetCenter();
	}

	CachedLocalCenter = GetActorTransform().InverseTransformPosition(WorldCenter);
}


void AMotionMatchingTraversableBlock::CollectLedges()
{
	LedgeSplines.Reset();

	TArray<USplineComponent*> SplineComponents;
	GetComponents<USplineComponent>(SplineComponents);

	for (USplineComponent* SplineComponent : SplineComponents)
	{
		if (IsValid(SplineComponent) == false) continue;

		if (LedgeComponentTag != NAME_None)
		{
			if (SplineComponent->ComponentHasTag(LedgeComponentTag) == false) continue;
		}

		LedgeSplines.Add(SplineComponent);
	}
}


void AMotionMatchingTraversableBlock::PairLedges()
{
	OppositeLedgeMap.Reset();

	if (bIsAutoPairOppositeLedges == true)
	{
		PairLedgesAutomatically();
		return;
	}

	PairLedgesManually();
}


void AMotionMatchingTraversableBlock::PairLedgesManually()
{
	for (const FMotionMatchingLedgePair& LedgePair : ManualLedgePairs)
	{
		USplineComponent* Ledge = FindLedgeByName(LedgePair.LedgeName);
		USplineComponent* OppositeLedge = FindLedgeByName(LedgePair.OppositeLedgeName);

		if (IsValid(Ledge) == false) continue;
		if (IsValid(OppositeLedge) == false) continue;
		if (Ledge == OppositeLedge) continue;

		OppositeLedgeMap.Add(Ledge, OppositeLedge);
		OppositeLedgeMap.Add(OppositeLedge, Ledge);
	}
}


void AMotionMatchingTraversableBlock::PairLedgesAutomatically()
{
	for (USplineComponent* Ledge : LedgeSplines)
	{
		if (IsValid(Ledge) == false) continue;

		USplineComponent* OppositeLedge = FindOppositeLedgeAutomatically(Ledge);
		if (IsValid(OppositeLedge) == false) continue;

		OppositeLedgeMap.Add(Ledge, OppositeLedge);
	}
}


// 노멀이 반대 방향(-0.7 이하)이고 내 뒤쪽에 있는 Ledge 중 가장 가까운 것을 반대편 Ledge로 선택
USplineComponent* AMotionMatchingTraversableBlock::FindOppositeLedgeAutomatically(USplineComponent* Ledge) const
{
	if (IsValid(Ledge) == false) return nullptr;

	const FVector LedgeMiddleLocation = GetLedgeMiddleLocation(Ledge);
	const FVector LedgeNormal = GetLedgeNormal(Ledge, LedgeMiddleLocation);

	USplineComponent* ClosestOppositeLedge = nullptr;
	float ClosestDistance = TNumericLimits<float>::Max();

	for (USplineComponent* OtherLedge : LedgeSplines)
	{
		if (IsValid(OtherLedge) == false) continue;
		if (OtherLedge == Ledge) continue;

		const FVector OtherLedgeMiddleLocation = GetLedgeMiddleLocation(OtherLedge);
		const FVector OtherLedgeNormal = GetLedgeNormal(OtherLedge, OtherLedgeMiddleLocation);

		if (FVector::DotProduct(LedgeNormal, OtherLedgeNormal) > -0.7f) continue;

		const FVector LedgeToOtherLedge = OtherLedgeMiddleLocation - LedgeMiddleLocation;
		if (FVector::DotProduct(LedgeToOtherLedge.GetSafeNormal2D(), -LedgeNormal) < 0.5f) continue;

		const float Distance = LedgeToOtherLedge.Size2D();
		if (Distance >= ClosestDistance) continue;

		ClosestDistance = Distance;
		ClosestOppositeLedge = OtherLedge;
	}

	return ClosestOppositeLedge;
}


USplineComponent* AMotionMatchingTraversableBlock::FindLedgeByName(const FName& LedgeName) const
{
	for (USplineComponent* Ledge : LedgeSplines)
	{
		if (IsValid(Ledge) == false) continue;
		if (Ledge->GetFName() == LedgeName) return Ledge;
	}

	return nullptr;
}


void AMotionMatchingTraversableBlock::ApplyTraversableCollision()
{
	TArray<UPrimitiveComponent*> PrimitiveComponents;
	GetComponents<UPrimitiveComponent>(PrimitiveComponents);

	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (IsValid(PrimitiveComponent) == false) continue;
		if (PrimitiveComponent->IsA<USplineComponent>() == true) continue;
		if (PrimitiveComponent->GetCollisionEnabled() == ECollisionEnabled::NoCollision) continue;

		PrimitiveComponent->SetCollisionResponseToChannel(MotionMatchingCollision::Traversable, ECR_Block);
	}
}


FVector AMotionMatchingTraversableBlock::GetObstacleCenter() const
{
	return GetActorTransform().TransformPosition(CachedLocalCenter);
}


FVector AMotionMatchingTraversableBlock::GetLedgeMiddleLocation(const USplineComponent* Ledge) const
{
	if (IsValid(Ledge) == false) return FVector::ZeroVector;

	const float HalfLength = Ledge->GetSplineLength() * 0.5f;
	return Ledge->GetLocationAtDistanceAlongSpline(HalfLength, ESplineCoordinateSpace::World);
}


FVector AMotionMatchingTraversableBlock::GetLedgeNormal(const USplineComponent* Ledge, const FVector& WorldLocation) const
{
	if (IsValid(Ledge) == false) return FVector::ZeroVector;

	if (bIsSplineUpVectorUsedAsNormal == true)
	{
		return Ledge->FindUpVectorClosestToWorldLocation(WorldLocation, ESplineCoordinateSpace::World);
	}

	const FVector LedgeDirection = Ledge->FindDirectionClosestToWorldLocation(WorldLocation, ESplineCoordinateSpace::World);
	FVector LedgeNormal = FVector::CrossProduct(LedgeDirection, FVector::UpVector).GetSafeNormal();

	const FVector CenterToLocation = (WorldLocation - GetObstacleCenter()).GetSafeNormal2D();
	if (FVector::DotProduct(LedgeNormal, CenterToLocation) < 0.0f)
	{
		LedgeNormal *= -1.0f;
	}

	return LedgeNormal;
}


USplineComponent* AMotionMatchingTraversableBlock::FindLedgeClosestToActor(const FVector& ActorLocation) const
{
	USplineComponent* ClosestLedge = nullptr;
	float ClosestDistance = TNumericLimits<float>::Max();

	for (USplineComponent* Ledge : LedgeSplines)
	{
		if (IsValid(Ledge) == false) continue;

		FVector LedgeLocation = Ledge->FindLocationClosestToWorldLocation(ActorLocation, ESplineCoordinateSpace::World);
		LedgeLocation += GetLedgeNormal(Ledge, LedgeLocation) * 10.0f;

		const float Distance = FVector::Dist(LedgeLocation, ActorLocation);
		if (Distance >= ClosestDistance) continue;

		ClosestDistance = Distance;
		ClosestLedge = Ledge;
	}

	return ClosestLedge;
}


// 캐릭터와 가장 가까운 Ledge(앞)와 그 반대편 Ledge(뒤)의 위치/노멀을 계산
FMotionMatchingLedgeResult AMotionMatchingTraversableBlock::ComputeLedgeTransforms(const FVector& HitLocation, const FVector& ActorLocation) const
{
	FMotionMatchingLedgeResult LedgeResult;

	USplineComponent* FrontLedge = FindLedgeClosestToActor(ActorLocation);
	if (IsValid(FrontLedge) == false) return LedgeResult;

	const float LedgeLength = FrontLedge->GetSplineLength();
	if (LedgeLength < MinLedgeWidth) return LedgeResult;

	const FVector LocationClosestToHit = FrontLedge->FindLocationClosestToWorldLocation(HitLocation, ESplineCoordinateSpace::World);
	const float DistanceAlongSpline = FrontLedge->GetDistanceAlongSplineAtLocation(LocationClosestToHit, ESplineCoordinateSpace::World);
	const float HalfLedgeWidth = MinLedgeWidth * 0.5f;
	const float ClampedDistance = FMath::Clamp(DistanceAlongSpline, HalfLedgeWidth, LedgeLength - HalfLedgeWidth);

	const FTransform FrontLedgeTransform = FrontLedge->GetTransformAtDistanceAlongSpline(ClampedDistance, ESplineCoordinateSpace::World);
	LedgeResult.bIsFrontLedgeFound = true;
	LedgeResult.FrontLedgeLocation = FrontLedgeTransform.GetLocation();
	LedgeResult.FrontLedgeNormal = GetLedgeNormal(FrontLedge, LedgeResult.FrontLedgeLocation);

	USplineComponent* BackLedge = OppositeLedgeMap.FindRef(FrontLedge);
	if (IsValid(BackLedge) == false) return LedgeResult;

	const FTransform BackLedgeTransform = BackLedge->FindTransformClosestToWorldLocation(LedgeResult.FrontLedgeLocation, ESplineCoordinateSpace::World);
	LedgeResult.bIsBackLedgeFound = true;
	LedgeResult.BackLedgeLocation = BackLedgeTransform.GetLocation();
	LedgeResult.BackLedgeNormal = GetLedgeNormal(BackLedge, LedgeResult.BackLedgeLocation);

	return LedgeResult;
}


#if WITH_EDITOR
void AMotionMatchingTraversableBlock::DebugDrawLedges()
{
	RefreshLedges();

	UWorld* World = GetWorld();
	if (IsValid(World) == false) return;

	const float DrawDuration = 5.0f;

	for (USplineComponent* Ledge : LedgeSplines)
	{
		if (IsValid(Ledge) == false) continue;

		const FVector LedgeMiddleLocation = GetLedgeMiddleLocation(Ledge);
		const FVector LedgeNormal = GetLedgeNormal(Ledge, LedgeMiddleLocation);

		FColor NormalColor = FColor::Green;
		if (Ledge->GetSplineLength() < MinLedgeWidth)
		{
			NormalColor = FColor::Red;
		}

		DrawDebugDirectionalArrow(World, LedgeMiddleLocation, LedgeMiddleLocation + LedgeNormal * 50.0f, 20.0f, NormalColor, false, DrawDuration, 0, 2.0f);

		const USplineComponent* OppositeLedge = OppositeLedgeMap.FindRef(Ledge);
		if (IsValid(OppositeLedge) == false) continue;

		const FVector OppositeLedgeMiddleLocation = GetLedgeMiddleLocation(OppositeLedge);
		DrawDebugLine(World, LedgeMiddleLocation, OppositeLedgeMiddleLocation, FColor::Yellow, false, DrawDuration, 0, 1.0f);
	}
}
#endif
