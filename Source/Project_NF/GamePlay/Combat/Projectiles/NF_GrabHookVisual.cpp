#include "GamePlay/Combat/Projectiles/NF_GrabHookVisual.h"
#include "GamePlay/Combat/Projectiles/NF_GrabHook.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ANF_GrabHookVisual::ANF_GrabHookVisual()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	HookMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HookMesh"));
	HookMesh->SetupAttachment(RootComponent);
	HookMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HookMesh->SetRelativeScale3D(FVector(0.6f));
	TetherMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TetherMesh"));
	TetherMesh->SetupAttachment(RootComponent);
	TetherMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TetherMesh->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Sphere.Succeeded()) { HookMesh->SetStaticMesh(Sphere.Object); }
	if (Cylinder.Succeeded()) { TetherMesh->SetStaticMesh(Cylinder.Object); }
}

void ANF_GrabHookVisual::Initialize(ANF_GrabHook* Hook, AActor* Source)
{
	Simulation = Hook; SourceActor = Source;
	if (Hook && Source)
	{
		FVector Eyes; FRotator Aim; Source->GetActorEyesViewPoint(Eyes, Aim);
		FallbackOffset = Source->GetActorQuat().UnrotateVector(Hook->GetSourceAnchor(Source) - Eyes);
	}
}

FVector ANF_GrabHookVisual::GetAnchor() const
{
	AActor* Source = SourceActor.Get();
	if (!Source) { return GetActorLocation(); }
	TInlineComponentArray<USkeletalMeshComponent*> Meshes; Source->GetComponents(Meshes);
	for (auto* Mesh : Meshes)
	{
		if ((!SourceMeshComponentName.IsNone() && Mesh->GetFName() != SourceMeshComponentName)
			|| (SourceMeshComponentName.IsNone() && Cast<ACharacter>(Source) && Mesh != Cast<ACharacter>(Source)->GetMesh())) { continue; }
		if (!SourceSocketName.IsNone() && Mesh->DoesSocketExist(SourceSocketName)) { return Mesh->GetSocketLocation(SourceSocketName); }
	}
	if (Simulation.IsValid()) { return Simulation->GetSourceAnchor(Source); }
	FVector Eyes; FRotator Aim; Source->GetActorEyesViewPoint(Eyes, Aim);
	return Eyes + Source->GetActorQuat().RotateVector(FallbackOffset);
}

void ANF_GrabHookVisual::BeginReturn()
{
	if (bReturning) { return; }
	bReturning = true;
	SetLifeSpan(5.0f);
}

void ANF_GrabHookVisual::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!SourceActor.IsValid() || SourceActor->IsActorBeingDestroyed()) { Destroy(); return; }
	if (!Simulation.IsValid() || Simulation->IsActorBeingDestroyed()
		|| Simulation->GetHookPhase() == ENF_GrabHookPhase::Returning
		|| Simulation->GetHookPhase() == ENF_GrabHookPhase::Finished) { BeginReturn(); }
	const FVector Anchor = GetAnchor();
	if (bReturning)
	{
		if (FVector::DistSquared(GetActorLocation(), Anchor) <= FMath::Square(FMath::Max(10.0f, ReturnSpeed * DeltaSeconds))) { Destroy(); return; }
		SetActorLocation(FMath::VInterpConstantTo(GetActorLocation(), Anchor, DeltaSeconds, FMath::Max(1.0f, ReturnSpeed)));
	}
	else { SetActorLocationAndRotation(Simulation->GetActorLocation(), Simulation->GetActorRotation()); }
	const FVector Difference = GetActorLocation() - Anchor;
	const float Length = Difference.Size();
	TetherMesh->SetVisibility(bShowTether && Length > 1.0f);
	if (bShowTether && Length > 1.0f)
	{
		TetherMesh->SetWorldLocation(Anchor + Difference * 0.5f);
		TetherMesh->SetWorldRotation(FRotationMatrix::MakeFromZ(Difference).Rotator());
		TetherMesh->SetWorldScale3D(FVector(TetherRadius / 50.0f, TetherRadius / 50.0f, Length / 100.0f));
	}
}
