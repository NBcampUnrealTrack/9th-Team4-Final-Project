#include "GamePlay/Combat/Components/NF_GroundTravelComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Math/RotationMatrix.h"

UNF_GroundTravelComponent::UNF_GroundTravelComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bAutoActivate = false;
}

bool UNF_GroundTravelComponent::HasValidSettings() const
{
	return FMath::IsFinite(TravelSpeed) && TravelSpeed > 0 && FMath::IsFinite(MaxTravelDistance) && MaxTravelDistance > 0
		&& FMath::IsFinite(InitialBoostMultiplier) && InitialBoostMultiplier >= 1.0f
		&& FMath::IsFinite(TravelSpeed * InitialBoostMultiplier) && FMath::IsFinite(BoostDuration) && BoostDuration >= 0.0f
		&& FMath::IsFinite(SpawnForwardDistance) && SpawnForwardDistance > 0
		&& FMath::IsFinite(GroundSearchDepth) && GroundSearchDepth > 0 && FMath::IsFinite(StepHeight) && StepHeight > 0
		&& FMath::IsFinite(GroundClearance) && GroundClearance > 0
		&& FMath::IsFinite(MinGroundNormalZ) && MinGroundNormalZ > 0 && MinGroundNormalZ < 1;
}

float UNF_GroundTravelComponent::GetTravelSpeedAtTime(float ElapsedSeconds) const
{
	if (BoostDuration <= 0.0f) { return TravelSpeed; }
	const float Remaining = 1.0f - FMath::Clamp(ElapsedSeconds / BoostDuration, 0.0f, 1.0f);
	return TravelSpeed * (1.0f + (InitialBoostMultiplier - 1.0f) * Remaining * Remaining);
}

float UNF_GroundTravelComponent::CalculateTravelDistance(float StartTime, float StepSeconds) const
{
	if (BoostDuration <= 0.0f) { return TravelSpeed * StepSeconds; }
	// Integrate quadratic decay, including steps crossing boost expiry, without frame partition bias.
	const double Duration = BoostDuration;
	const double Before = 1.0 - FMath::Clamp(static_cast<double>(StartTime) / Duration, 0.0, 1.0);
	const double After = 1.0 - FMath::Clamp((static_cast<double>(StartTime) + StepSeconds) / Duration, 0.0, 1.0);
	const double BoostDistance = TravelSpeed * (InitialBoostMultiplier - 1.0) * Duration / 3.0
		* (Before * Before * Before - After * After * After);
	return static_cast<float>(TravelSpeed * static_cast<double>(StepSeconds) + BoostDistance);
}

bool UNF_GroundTravelComponent::ConfigureComponents()
{
	AActor* Owner = GetOwner();
	USphereComponent* Collision = Owner ? Owner->FindComponentByClass<USphereComponent>() : nullptr;
	if (!Collision) { return false; }
	Sphere = Collision;
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Collision->SetSimulatePhysics(false);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Block);
	Collision->SetGenerateOverlapEvents(false);
	Collision->SetCanEverAffectNavigation(false);
	USceneComponent* OldRoot = Owner->GetRootComponent();
	if (OldRoot != Collision)
	{
		Collision->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		Owner->SetRootComponent(Collision);
		if (OldRoot) { OldRoot->AttachToComponent(Collision, FAttachmentTransformRules::KeepWorldTransform); }
	}
	TArray<UStaticMeshComponent*> Meshes; Owner->GetComponents(Meshes);
	for (auto* Mesh : Meshes) { Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
	SetUpdatedComponent(Collision);
	return true;
}

bool UNF_GroundTravelComponent::FindGround(FVector Point, FHitResult& Hit) const
{
	if (!GetWorld()) { return false; }
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NF_GroundTravel), false, GetOwner());
	if (Caster.IsValid()) { Params.AddIgnoredActor(Caster.Get()); }
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic); Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	return GetWorld()->LineTraceSingleByObjectType(Hit, Point + FVector::UpVector * StepHeight,
		Point - FVector::UpVector * GroundSearchDepth, Objects, Params) && Hit.ImpactNormal.Z >= MinGroundNormalZ;
}

bool UNF_GroundTravelComponent::CalculateStart(AActor* Avatar, FVector AimDirection, float Radius, FVector& Location, FVector& Direction) const
{
	if (!HasValidSettings() || !IsValid(Avatar) || !Avatar->GetWorld() || AimDirection.ContainsNaN()
		|| !FMath::IsFinite(Radius) || Radius <= 0) { return false; }
	Direction = AimDirection.GetSafeNormal2D();
	if (Direction.IsNearlyZero()) { Direction = Avatar->GetActorForwardVector().GetSafeNormal2D(); }
	if (Direction.IsNearlyZero()) { return false; }
	FVector Feet = Avatar->GetActorLocation();
	if (const ACharacter* Character = Cast<ACharacter>(Avatar))
	{
		Feet.Z -= Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NF_GroundTravelSpawn), false, Avatar);
	if (GetOwner() && GetOwner() != Avatar) { Params.AddIgnoredActor(GetOwner()); }
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic); Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	const FVector Point = Feet + Direction * SpawnForwardDistance;
	FHitResult Floor;
	if (!Avatar->GetWorld()->LineTraceSingleByObjectType(Floor, Point + FVector::UpVector * StepHeight,
		Point - FVector::UpVector * GroundSearchDepth, Objects, Params) || Floor.ImpactNormal.Z < MinGroundNormalZ) { return false; }
	Location = Floor.ImpactPoint + Floor.ImpactNormal * (Radius + GroundClearance);
	FHitResult Obstacle;
	return !Avatar->GetWorld()->SweepSingleByChannel(Obstacle, Feet + FVector::UpVector * (Radius + GroundClearance),
		Location, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(Radius), Params);
}

bool UNF_GroundTravelComponent::PrepareTravel(AActor* Avatar, FVector AimDirection)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Sphere.IsValid()) { return false; }
	FVector Location;
	if (!CalculateStart(Avatar, AimDirection, Sphere->GetScaledSphereRadius(), Location, Heading)) { return false; }
	Caster = Avatar;
	Sphere->IgnoreActorWhenMoving(Avatar, true);
	GetOwner()->SetActorLocationAndRotation(Location, Heading.Rotation());
	bPrepared = true; return true;
}

bool UNF_GroundTravelComponent::StartTravel()
{
	if (!bPrepared || !GetOwner()->HasAuthority() || !Sphere.IsValid()) { return false; }
	DistanceTravelled = 0; TravelElapsed = 0; bTravelling = true;
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Activate(true); SetComponentTickEnabled(true); return true;
}

void UNF_GroundTravelComponent::StopTravel()
{
	bTravelling = false; Velocity = FVector::ZeroVector;
	UpdateComponentVelocity(); SetComponentTickEnabled(false);
	if (Sphere.IsValid()) { Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
}

void UNF_GroundTravelComponent::FinishTravel()
{
	if (!bTravelling) { return; }
	StopTravel(); OnTravelEnded.Broadcast();
}

void UNF_GroundTravelComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bTravelling || !GetOwner()->HasAuthority() || !Sphere.IsValid() || !UpdatedComponent) { return; }
	float Remaining = FMath::Clamp(DeltaTime, 0.0f, 0.25f);
	while (Remaining > SMALL_NUMBER && bTravelling)
	{
		const float Step = FMath::Min(Remaining, 0.016f); Remaining -= Step;
		const FVector Before = UpdatedComponent->GetComponentLocation();
		FHitResult Floor;
		if (!FindGround(Before, Floor)) { FinishTravel(); break; }
		const FVector Direction = FVector::VectorPlaneProject(Heading, Floor.ImpactNormal).GetSafeNormal();
		const float Distance = FMath::Min(CalculateTravelDistance(TravelElapsed, Step), MaxTravelDistance - DistanceTravelled);
		TravelElapsed += Step;
		FHitResult NextFloor;
		if (!FindGround(Before + Direction * Distance, NextFloor)) { FinishTravel(); break; }
		const FVector Target = NextFloor.ImpactPoint + NextFloor.ImpactNormal * (Sphere->GetScaledSphereRadius() + GroundClearance);
		FHitResult Hit;
		Velocity = (Target - Before) / Step;
		SafeMoveUpdatedComponent(Target - Before, FRotationMatrix::MakeFromXZ(Direction, NextFloor.ImpactNormal).ToQuat(), true, Hit);
		if (Hit.IsValidBlockingHit())
		{
			if ((Hit.GetActor() && Hit.GetActor()->IsA<APawn>()) || Hit.ImpactNormal.Z < MinGroundNormalZ) { FinishTravel(); break; }
			SlideAlongSurface(Target - Before, 1.0f - Hit.Time, Hit.Normal, Hit, true);
		}
		if (!bTravelling || !UpdatedComponent) { break; }
		DistanceTravelled += FVector::Distance(Before, UpdatedComponent->GetComponentLocation());
		UpdateComponentVelocity();
		if (DistanceTravelled >= MaxTravelDistance - 0.1f || Hit.bStartPenetrating
			|| FVector::DistSquared(Before, UpdatedComponent->GetComponentLocation()) < 0.001f) { FinishTravel(); }
	}
}
