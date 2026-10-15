#include "GamePlay/Combat/Components/NF_ProjectileLaunchComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/ProjectileMovementComponent.h"

UNF_ProjectileLaunchComponent::UNF_ProjectileLaunchComponent() { PrimaryComponentTick.bCanEverTick = false; }

bool UNF_ProjectileLaunchComponent::HasValidSettings() const
{
	return FMath::IsFinite(ThrowSpeed) && ThrowSpeed > 0.0f && FMath::IsFinite(ThrowAngleOffset)
		&& FMath::Abs(ThrowAngleOffset) <= 80.0f && FMath::IsFinite(AimDistance) && AimDistance > 0.0f && !SpawnOffset.ContainsNaN();
}

bool UNF_ProjectileLaunchComponent::ConfigureComponents()
{
	AActor* Owner = GetOwner();
	if (!Owner) { return false; }
	Collision = Owner->FindComponentByClass<USphereComponent>();
	Movement = Owner->FindComponentByClass<UProjectileMovementComponent>();
	if (!Collision.IsValid() || !Movement.IsValid()) { return false; }
	Movement->Deactivate();
	Movement->StopMovementImmediately();
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Collision->SetSimulatePhysics(false);
	if (bConfigureCollision)
	{
		Collision->SetCollisionObjectType(ECC_WorldDynamic);
		Collision->SetCollisionResponseToAllChannels(ECR_Block);
		Collision->SetGenerateOverlapEvents(false);
	}
	Collision->SetNotifyRigidBodyCollision(true);
	Collision->SetCanEverAffectNavigation(false);
	TArray<UStaticMeshComponent*> Meshes;
	Owner->GetComponents(Meshes);
	for (auto* Mesh : Meshes) { Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
	// 부모의 SceneRoot를 유지하면서 Sphere를 실제 이동 루트로 승격한다.
	USceneComponent* OldRoot = Owner->GetRootComponent();
	if (OldRoot != Collision.Get())
	{
		FTransform Transform = Owner->GetActorTransform();
		Transform.SetScale3D(Collision->GetComponentScale());
		Collision->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		Owner->SetRootComponent(Collision.Get());
		Collision->SetWorldTransform(Transform);
		if (OldRoot) { OldRoot->AttachToComponent(Collision.Get(), FAttachmentTransformRules::KeepWorldTransform); }
	}
	Movement->SetUpdatedComponent(Collision.Get());
	Movement->bInitialVelocityInLocalSpace = false;
	Movement->InitialSpeed = 0.0f;
	return true;
}

bool UNF_ProjectileLaunchComponent::CalculateLaunch(AActor* Avatar, FVector AimDirection, float CollisionRadius,
	float MaxSpeed, FVector& Location, FVector& Velocity) const
{
	if (!HasValidSettings() || !IsValid(Avatar) || !Avatar->GetWorld() || AimDirection.ContainsNaN()
		|| AimDirection.IsNearlyZero() || !FMath::IsFinite(CollisionRadius) || CollisionRadius <= 0.0f
		|| !FMath::IsFinite(MaxSpeed) || MaxSpeed < 0.0f) { return false; }
	FVector Eyes;
	FRotator EyeRotation;
	Avatar->GetActorEyesViewPoint(Eyes, EyeRotation);
	const FRotator Aim = AimDirection.GetSafeNormal().Rotation();
	Location = Eyes + Aim.RotateVector(SpawnOffset);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NF_ComposedThrow), false, Avatar);
	if (GetOwner() && GetOwner() != Avatar) { Params.AddIgnoredActor(GetOwner()); }
	FHitResult Hit;
	const FVector AimEnd = Eyes + Aim.Vector() * AimDistance;
	const FVector AimPoint = Avatar->GetWorld()->LineTraceSingleByChannel(Hit, Eyes, AimEnd, ECC_Visibility, Params) ? Hit.ImpactPoint : AimEnd;
	FRotator Rotation = (AimPoint - Location).Rotation();
	Rotation.Pitch = FMath::Clamp(FRotator::NormalizeAxis(Rotation.Pitch) + ThrowAngleOffset, -80.0f, 80.0f);
	Velocity = Rotation.Vector() * (MaxSpeed > 0.0f ? FMath::Min(ThrowSpeed, MaxSpeed) : ThrowSpeed);
	return !Avatar->GetWorld()->SweepSingleByChannel(Hit, Eyes, Location, FQuat::Identity,
		ECC_Visibility, FCollisionShape::MakeSphere(CollisionRadius), Params);
}

bool UNF_ProjectileLaunchComponent::PrepareLaunch(AActor* Avatar, FVector AimDirection)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Collision.IsValid() || !Movement.IsValid()) { return false; }
	FVector Location;
	if (!CalculateLaunch(Avatar, AimDirection, Collision->GetScaledSphereRadius(), Movement->MaxSpeed, Location, InitialVelocity)) { return false; }
	GetOwner()->SetActorLocationAndRotation(Location, InitialVelocity.Rotation());
	bPrepared = true;
	return true;
}

bool UNF_ProjectileLaunchComponent::Launch(AActor* Avatar)
{
	if (!bPrepared || !GetOwner() || !GetOwner()->HasAuthority() || !Collision.IsValid() || !Movement.IsValid()) { return false; }
	if (IsValid(Avatar)) { Collision->IgnoreActorWhenMoving(Avatar, true); }
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Movement->SetUpdatedComponent(Collision.Get());
	Movement->Velocity = InitialVelocity;
	Movement->UpdateComponentVelocity();
	Movement->Activate(true);
	return true;
}

void UNF_ProjectileLaunchComponent::StopProjectile()
{
	if (Movement.IsValid())
	{
		Movement->StopMovementImmediately();
		Movement->SetUpdatedComponent(nullptr);
		Movement->Deactivate();
	}
	if (Collision.IsValid()) { Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
}
