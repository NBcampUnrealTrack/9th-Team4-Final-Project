#include "GamePlay/Combat/Projectiles/NF_GrabHook.h"
#include "Components/SphereComponent.h"
#include "GamePlay/Combat/Projectiles/NF_GrabHookVisual.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

ANF_GrabHook::ANF_GrabHook()
{
	bReplicates = true; SetReplicateMovement(true); SetNetUpdateFrequency(60.0f);
	PrimaryActorTick.bCanEverTick = true;
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("HookCollision")); SetRootComponent(Collision);
	Collision->InitSphereRadius(30.0f); Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Collision->SetCollisionObjectType(ECC_WorldDynamic); Collision->SetCollisionResponseToAllChannels(ECR_Block);
	Collision->SetGenerateOverlapEvents(false);
	Collision->OnComponentHit.AddDynamic(this, &ThisClass::HandleHit);
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetUpdatedComponent(Collision); ProjectileMovement->bAutoActivate = false;
	ProjectileMovement->ProjectileGravityScale = 0.0f; ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false; ProjectileMovement->bForceSubStepping = true;
	ProjectileMovement->OnProjectileStop.AddDynamic(this, &ThisClass::HandleProjectileStopped);
	// MovementComponent already ticks before its owner. Do not add the reverse dependency.
	VisualClass = ANF_GrabHookVisual::StaticClass();
}

bool ANF_GrabHook::HasValidSettings() const
{
	return FMath::IsFinite(FireSpeed) && FireSpeed > 0.0f
		&& !FallbackOffset.ContainsNaN();
}

FVector ANF_GrabHook::GetSourceAnchor(AActor* Source) const
{
	if (!IsValid(Source)) { return GetActorLocation(); }
	FVector Eyes; FRotator Aim; Source->GetActorEyesViewPoint(Eyes, Aim);
	return Eyes + Source->GetActorQuat().RotateVector(FallbackOffset);
}

bool ANF_GrabHook::StartFlight(AActor* Source, FVector Direction, float Range, float Radius, float MaximumHoldTime)
{
	if (!HasAuthority() || bLaunched || !HasValidSettings() || !IsValid(Source) || Source->GetWorld() != GetWorld()
		|| Direction.ContainsNaN() || !Direction.Normalize() || !FMath::IsFinite(Range) || Range <= 0.0f
		|| !FMath::IsFinite(Radius) || Radius <= 0.0f || !FMath::IsFinite(MaximumHoldTime) || MaximumHoldTime < 0.0f) { return false; }
	bLaunched = true; Frame.Source = Source; Frame.Phase = ENF_GrabHookPhase::Flying;
	FlightOrigin = GetActorLocation(); FlightDirection = Direction; FlightRange = Range;
	SetOwner(Source); Collision->SetSphereRadius(Radius);
	Collision->IgnoreActorWhenMoving(Source, true);
	TArray<AActor*> Attached; Source->GetAttachedActors(Attached, true, true);
	for (AActor* Actor : Attached) { Collision->IgnoreActorWhenMoving(Actor, true); }
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ProjectileMovement->SetUpdatedComponent(Collision);
	ProjectileMovement->InitialSpeed = FireSpeed; ProjectileMovement->MaxSpeed = FireSpeed;
	ProjectileMovement->Velocity = Direction * FireSpeed; ProjectileMovement->Activate(true);
	Deadline = GetWorld()->GetTimeSeconds() + Range / FireSpeed + MaximumHoldTime + 3.0f;
	OnRep_Frame(); ForceNetUpdate();
	return true;
}

void ANF_GrabHook::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority()) { ProjectileMovement->Deactivate(); Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
	UpdateVisuals();
}

void ANF_GrabHook::HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority() || Frame.Phase != ENF_GrabHookPhase::Flying || bPendingCharacterHit || OtherActor == Frame.Source) { return; }
	ProjectileMovement->StopMovementImmediately(); ProjectileMovement->Deactivate();
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (FVector::Distance(FlightOrigin, GetActorLocation()) > FlightRange + 1.0f)
	{
		UE_LOG(LogTemp, Display, TEXT("[Grab] Projectile impact rejected: beyond range"));
		SetActorLocation(FlightOrigin + FlightDirection * FlightRange); BeginReturn(); return;
	}
	ACharacter* Target = Cast<ACharacter>(OtherActor);
	if (Target)
	{
		// DispatchBlockingHit also notifies the struck Character. Defer until the owner tick,
		// after that reciprocal impact, so it cannot be mistaken for a wall hit during the pull.
		PendingHitTarget = Target; bPendingCharacterHit = true;
		PendingHitLocalOffset = Target->GetActorTransform().InverseTransformPosition(GetActorLocation());
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("[Grab] Projectile blocked by %s (component %s)"), *GetNameSafe(OtherActor), *GetNameSafe(OtherComponent));
		BeginReturn();
	}
}

void ANF_GrabHook::Latch(ACharacter* Target)
{
	if (!HasAuthority() || Frame.Phase != ENF_GrabHookPhase::Flying || !IsValid(Target)) { return; }
	Frame.Target = Target;
	Frame.TargetLocalOffset = Target->GetActorTransform().InverseTransformPosition(GetActorLocation());
	Frame.Phase = ENF_GrabHookPhase::Attached;
	ProjectileMovement->StopMovementImmediately(); ProjectileMovement->Deactivate();
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ForceNetUpdate(); UpdateVisuals();
}

void ANF_GrabHook::HandleProjectileStopped(const FHitResult& Hit)
{
	// ProjectileStop covers impacts suppressed by scoped component movement, too.
	HandleHit(Collision, Hit.GetActor(), Hit.GetComponent(), FVector::ZeroVector, Hit);
}

void ANF_GrabHook::BeginReturn()
{
	if (!HasAuthority() || Frame.Phase == ENF_GrabHookPhase::Finished || Frame.Phase == ENF_GrabHookPhase::Returning) { return; }
	bPendingCharacterHit = false; PendingHitTarget.Reset();
	if (Frame.Phase == ENF_GrabHookPhase::Attached && IsValid(Frame.Target))
	{
		SetActorLocation(Frame.Target->GetActorTransform().TransformPosition(Frame.TargetLocalOffset));
	}
	Frame.Target = nullptr; Frame.Phase = ENF_GrabHookPhase::Returning;
	ProjectileMovement->StopMovementImmediately(); ProjectileMovement->Deactivate();
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Deadline = GetWorld()->GetTimeSeconds() + 0.25f;
	ForceNetUpdate(); UpdateVisuals();
	// Retraction belongs to VisualActor. Notify gameplay now, not on visual arrival.
	if (!bFlightEndedNotified) { bFlightEndedNotified = true; OnFlightEnded.Broadcast(); }
	SetLifeSpan(0.25f);
}

void ANF_GrabHook::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority() && bLaunched && Frame.Phase != ENF_GrabHookPhase::Finished)
	{
		if (!IsValid(Frame.Source) || Frame.Source->IsActorBeingDestroyed() || GetWorld()->GetTimeSeconds() >= Deadline
			|| (CanContinue.IsBound() && !CanContinue.Execute())) { FinishReturn(); return; }
		if (Frame.Phase == ENF_GrabHookPhase::Flying && bPendingCharacterHit)
		{
			ACharacter* Target = PendingHitTarget.Get();
			bPendingCharacterHit = false; PendingHitTarget.Reset();
			if (IsValid(Target)) { SetActorLocation(Target->GetActorTransform().TransformPosition(PendingHitLocalOffset)); }
			const bool bAccepted = IsValid(Target) && OnCharacterHit.IsBound() && OnCharacterHit.Execute(Target);
			if (!bAccepted || Frame.Phase == ENF_GrabHookPhase::Flying) { BeginReturn(); }
		}
		if (Frame.Phase == ENF_GrabHookPhase::Flying)
		{
			const float Remaining = FlightRange - FVector::Distance(FlightOrigin, GetActorLocation());
			if (Remaining <= 1.0f)
			{
				if (Remaining < 0.0f) { SetActorLocation(FlightOrigin + FlightDirection * FlightRange); }
				BeginReturn();
			}
			else { ProjectileMovement->Velocity = FlightDirection * FMath::Min(FireSpeed, Remaining / FMath::Max(DeltaSeconds, 0.001f)); }
		}
	}
	if (Frame.Phase == ENF_GrabHookPhase::Attached)
	{
		if (IsValid(Frame.Target)) { SetActorLocation(Frame.Target->GetActorTransform().TransformPosition(Frame.TargetLocalOffset)); }
		else if (HasAuthority()) { BeginReturn(); }
	}
	UpdateVisuals();
}

void ANF_GrabHook::UpdateVisuals()
{
	if (GetNetMode() == NM_DedicatedServer || !IsValid(Frame.Source)) { return; }
	if (!bVisualSpawnAttempted && Frame.Phase != ENF_GrabHookPhase::Finished)
	{
		bVisualSpawnAttempted = true;
		if (VisualClass)
		{
			ANF_GrabHookVisual* Visual = GetWorld()->SpawnActor<ANF_GrabHookVisual>(VisualClass, GetActorTransform());
			VisualActor = Visual;
			if (Visual) { Visual->Initialize(this, Frame.Source); }
		}
	}
	if (VisualActor.IsValid() && (Frame.Phase == ENF_GrabHookPhase::Returning || Frame.Phase == ENF_GrabHookPhase::Finished))
	{
		VisualActor->BeginReturn();
	}
}

void ANF_GrabHook::OnRep_Frame()
{
	if (!HasAuthority()) { ProjectileMovement->Deactivate(); Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
	UpdateVisuals();
}

void ANF_GrabHook::FinishReturn()
{
	if (!HasAuthority() || Frame.Phase == ENF_GrabHookPhase::Finished) { return; }
	Frame.Phase = ENF_GrabHookPhase::Finished; Frame.Target = nullptr;
	bPendingCharacterHit = false; PendingHitTarget.Reset();
	ProjectileMovement->StopMovementImmediately(); ProjectileMovement->Deactivate();
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ForceNetUpdate(); UpdateVisuals();
	if (!bFlightEndedNotified) { bFlightEndedNotified = true; OnFlightEnded.Broadcast(); }
	SetLifeSpan(0.1f);
}

void ANF_GrabHook::Abort()
{
	if (!HasAuthority()) { return; }
	if (!bLaunched) { Destroy(); return; }
	FinishReturn();
}

void ANF_GrabHook::EndPlay(const EEndPlayReason::Type Reason)
{
	if (VisualActor.IsValid()) { VisualActor->BeginReturn(); }
	if (HasAuthority()) { FinishReturn(); }
	Super::EndPlay(Reason);
}

void ANF_GrabHook::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANF_GrabHook, Frame);
}
