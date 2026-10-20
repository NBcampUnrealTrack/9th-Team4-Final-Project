#include "GamePlay/Combat/Skill/Portal/NF_Portal.h"
#include "GamePlay/Combat/Skill/Portal/NF_PortalView.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

// @owner: 이정준 (구현: Codex) | 2026-10-15
// 목적: 서버에서만 이동을 실행하며 출구는 진입 판정 없이 표시용으로 사용한다.
ANF_Portal::ANF_Portal()
{
	bReplicates = true; SetReplicateMovement(true);
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;
	PortalViewClass = ANF_PortalView::StaticClass();
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("PortalRoot")));
	EntranceTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("EntranceTrigger"));
	EntranceTrigger->SetupAttachment(GetRootComponent());
	EntranceTrigger->InitBoxExtent(FVector(60, 100, 110));
	EntranceTrigger->SetRelativeLocation(FVector(0, 0, 110));
	EntranceTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	EntranceTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	EntranceTrigger->SetGenerateOverlapEvents(true);
	EntranceTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMesh"));
	PortalMesh->SetupAttachment(GetRootComponent());
	PortalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PortalMesh->SetRelativeLocation(FVector(0, 0, 110));
	PortalMesh->SetRelativeScale3D(FVector(0.08, 1.8, 2.2));
	PortalMesh->SetVisibility(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	PortalMesh->SetStaticMesh(Cube.Object);
	Deployment = CreateDefaultSubobject<UNF_DeployableComponent>(TEXT("Deployment"));
	Deployment->SetManagedStart(true);
}
void ANF_Portal::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// 루트는 지면, 트리거는 지면 바로 위에 배치한다.
	EntranceTrigger->SetRelativeLocation(FVector(0, 0, EntranceTrigger->GetUnscaledBoxExtent().Z + 2.0f));
}
bool ANF_Portal::HasValidSettings() const
{
	return Deployment && Deployment->HasValidSettings() && EntranceTrigger
		&& !EntranceTrigger->GetUnscaledBoxExtent().ContainsNaN() && EntranceTrigger->GetUnscaledBoxExtent().GetMin() > 0.0
		&& FMath::IsFinite(ExitForwardOffset) && ExitForwardOffset > 0.0f
		&& FMath::IsFinite(ArrivalSearchHeight) && ArrivalSearchHeight > 0.0f
		&& FMath::IsFinite(ArrivalSearchDepth) && ArrivalSearchDepth > 0.0f
		&& FMath::IsFinite(GroundClearance) && GroundClearance >= 1.0f
		&& FMath::IsFinite(ReentryCooldown) && ReentryCooldown >= 0.1f;
}
void ANF_Portal::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		Deployment->OnDeploymentEnded.AddDynamic(this, &ThisClass::HandleDeploymentEnded);
		EntranceTrigger->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleBeginOverlap);
	}
	RefreshPortalState();
}
void ANF_Portal::InitializePortal(ANF_Portal* Other, bool bIsExit, ACharacter* Caster)
{
	if (!HasAuthority() || bActive || bClosing || !IsValid(Other) || Other == this) { return; }
	LinkedPortal = Other; bExitPortal = bIsExit; SourceCharacter = Caster;
}
bool ANF_Portal::CanPlaceFor(ACharacter* Character) const
{
	if (!GetWorld() || !IsValid(Character) || !Character->GetCapsuleComponent() || !HasValidSettings()) { return false; }
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NF_PortalPlacement), false, Character);
	Params.AddIgnoredActor(this); Params.AddIgnoredActor(LinkedPortal);
	return !GetWorld()->OverlapBlockingTestByProfile(EntranceTrigger->GetComponentLocation(), EntranceTrigger->GetComponentQuat(),
		Character->GetCapsuleComponent()->GetCollisionProfileName(), FCollisionShape::MakeBox(EntranceTrigger->GetScaledBoxExtent()), Params);
}
bool ANF_Portal::FindArrivalLocation(ACharacter* Character, FVector& OutLocation) const
{
	OutLocation = FVector::ZeroVector;
	if (!GetWorld() || !IsValid(Character) || !Character->GetCapsuleComponent() || !Character->GetCharacterMovement() || !HasValidSettings()) { return false; }
	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	const FVector Foot = GetActorLocation() + GetActorForwardVector() * ExitForwardOffset;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NF_PortalArrival), false, Character);
	Params.AddIgnoredActor(this); Params.AddIgnoredActor(LinkedPortal);
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic); Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	FHitResult Floor;
	if (!GetWorld()->LineTraceSingleByObjectType(Floor, Foot + FVector::UpVector * ArrivalSearchHeight,
		Foot - FVector::UpVector * ArrivalSearchDepth, Objects, Params) || !Character->GetCharacterMovement()->IsWalkable(Floor)) { return false; }
	if (FMath::Abs(Floor.ImpactPoint.Z - Foot.Z) > Character->GetCharacterMovement()->MaxStepHeight) { return false; }
	const FVector Candidate = Floor.ImpactPoint + FVector::UpVector * (Capsule->GetScaledCapsuleHalfHeight() + GroundClearance);
	if (GetWorld()->OverlapBlockingTestByProfile(Candidate, Capsule->GetComponentQuat(), Capsule->GetCollisionProfileName(),
		FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight()), Params)) { return false; }
	OutLocation = Candidate;
	return !OutLocation.ContainsNaN();
}
bool ANF_Portal::StartPortal()
{
	if (!HasAuthority() || bClosing || bActive || !IsValid(LinkedPortal) || LinkedPortal->bClosing
		|| (!bExitPortal && !LinkedPortal->bActive) || !HasValidSettings() || !Deployment->StartDeployment()) { return false; }
	if (bClosing || IsActorBeingDestroyed()) { return false; }
	bActive = true; RefreshPortalState(); ForceNetUpdate();
	if (!bExitPortal && bActive && !bClosing)
	{
		GetWorld()->GetTimerManager().SetTimer(RetryTimer, this, &ThisClass::RetryOverlappingPlayers, 0.1f, true);
	}
	return bActive && !bClosing;
}
void ANF_Portal::HandleBeginOverlap(UPrimitiveComponent* Component, AActor* Other, UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) { TryTeleport(Cast<ACharacter>(Other)); }
void ANF_Portal::RetryOverlappingPlayers()
{
	if (!HasAuthority() || bClosing || !bActive || bExitPortal) { return; }
	const double Now = GetWorld()->GetTimeSeconds();
	for (auto It = Cooldowns.CreateIterator(); It; ++It) { if (!It.Key().IsValid() || It.Value() <= Now) { It.RemoveCurrent(); } }
	TArray<AActor*> Overlaps; EntranceTrigger->GetOverlappingActors(Overlaps, ACharacter::StaticClass());
	for (AActor* Actor : Overlaps)
	{
		if (bClosing || !bActive) { break; }
		TryTeleport(Cast<ACharacter>(Actor));
	}
}
bool ANF_Portal::TryTeleport(ACharacter* Character)
{
	if (!HasAuthority() || bClosing || !bActive || bExitPortal || !IsValid(Character) || Character->IsActorBeingDestroyed()
		|| !IsValid(LinkedPortal) || !LinkedPortal->bActive || LinkedPortal->bClosing
		|| (!Character->IsPlayerControlled() && !Character->GetPlayerState())
		|| (bOnlyCasterCanUse && Character != SourceCharacter.Get())
		|| !Character->GetCharacterMovement() || Character->GetCharacterMovement()->MovementMode == MOVE_None) { return false; }
	const TWeakObjectPtr<ACharacter> Key(Character);
	const double Now = GetWorld()->GetTimeSeconds();
	if (const double* Until = Cooldowns.Find(Key)) { if (*Until > Now) { return false; } }
	FVector Destination;
	if (!LinkedPortal->FindArrivalLocation(Character, Destination)) { return false; }
	// 예약을 먼저 기록해 TeleportTo의 동기 Overlap 콜백 재진입을 막는다.
	Cooldowns.Add(Key, Now + ReentryCooldown);
	const FRotator Rotation = bAlignViewToExit ? FRotator(0, LinkedPortal->GetActorRotation().Yaw, 0) : Character->GetActorRotation();
	if (!Character->TeleportTo(Destination, Rotation)) { Cooldowns.Remove(Key); return false; }
	if (!IsValid(Character) || Character->IsActorBeingDestroyed()) { return true; }
	if (!bPreserveVelocity) { Character->GetCharacterMovement()->StopMovementImmediately(); }
	if (bAlignViewToExit)
	{
		if (AController* Controller = Character->GetController())
		{
			FRotator View = Controller->GetControlRotation(); View.Yaw = Rotation.Yaw;
			Controller->SetControlRotation(View);
			if (APlayerController* PlayerController = Cast<APlayerController>(Controller)) { PlayerController->ClientSetRotation(View, false); }
		}
	}
	Character->ForceNetUpdate();
	return true;
}
void ANF_Portal::ClosePortalPair()
{
	if (!HasAuthority() || bClosing) { return; }
	bClosing = true; bActive = false;
	if (GetWorld()) { GetWorld()->GetTimerManager().ClearTimer(RetryTimer); }
	RefreshPortalState(); ForceNetUpdate();
	if (IsValid(LinkedPortal)) { LinkedPortal->ClosePortalPair(); }
	Deployment->EndDeployment();
	if (!IsActorBeingDestroyed() && GetLifeSpan() <= 0.0f) { SetLifeSpan(0.5f); }
}
void ANF_Portal::HandleDeploymentEnded(ENF_DeploymentEndReason Reason) { ClosePortalPair(); }
void ANF_Portal::OnRep_PortalState() { if (HasActorBegunPlay()) { RefreshPortalState(); } }
void ANF_Portal::RefreshPortalState()
{
	EntranceTrigger->SetCollisionEnabled(HasAuthority() && bActive && !bExitPortal ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	PortalMesh->SetVisibility(bActive, true);
	if (UMaterialInterface* Material = bExitPortal ? ExitMaterial.Get() : EntranceMaterial.Get()) { PortalMesh->SetMaterial(0, Material); }
	if (GetNetMode() != NM_DedicatedServer) { PortalStateChanged(bExitPortal, bActive); }
	RefreshLocalViews();
}

FTransform ANF_Portal::GetViewTransform() const
{
	// Both gameplay actors face the travel direction. Their visible mouths face opposite sides:
	// entrance toward the caster, exit toward the arrival area. This preserves the existing travel heading.
	const FQuat MouthRotation = GetActorQuat() * FQuat(FVector::UpVector, bExitPortal ? 0.0f : PI);
	return FTransform(MouthRotation, PortalMesh->GetComponentLocation());
}

void ANF_Portal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	RefreshLocalViews();
}

void ANF_Portal::ClearLocalViews()
{
	for (auto& Pair : LocalViews) { if (Pair.Value.IsValid()) { Pair.Value->Destroy(); } }
	LocalViews.Reset();
}

void ANF_Portal::RefreshLocalViews()
{
	if (GetNetMode() == NM_DedicatedServer) { return; }
	if (!bActive || !bEnablePortalView || !PortalViewClass || !IsValid(LinkedPortal) || !LinkedPortal->IsPortalActive() || bClosing)
	{
		ClearLocalViews(); PortalMesh->SetVisibility(bActive, true); return;
	}
	UGameInstance* Instance = GetGameInstance();
	if (!Instance) { return; }
	for (auto It = LocalViews.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid() || !Instance->GetLocalPlayers().Contains(It.Key().Get()) || !It.Value().IsValid())
		{
			if (It.Value().IsValid()) { It.Value()->Destroy(); }
			It.RemoveCurrent();
		}
	}
	for (ULocalPlayer* Player : Instance->GetLocalPlayers())
	{
		APlayerController* Controller = Player ? Player->GetPlayerController(GetWorld()) : nullptr;
		if (!Controller || !Controller->IsLocalController() || LocalViews.Contains(Player)) { continue; }
		FActorSpawnParameters Params; Params.Owner = Controller->GetViewTarget();
		if (ANF_PortalView* View = GetWorld()->SpawnActor<ANF_PortalView>(PortalViewClass, GetViewTransform(), Params))
		{
			View->Initialize(this, Controller); LocalViews.Add(Player, View);
		}
	}
	// Replace the opaque placeholder slab only where a local view surface exists.
	PortalMesh->SetVisibility(bActive && LocalViews.IsEmpty(), true);
}
void ANF_Portal::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANF_Portal, LinkedPortal);
	DOREPLIFETIME(ANF_Portal, bExitPortal);
	DOREPLIFETIME(ANF_Portal, bActive);
}
void ANF_Portal::EndPlay(const EEndPlayReason::Type Reason)
{
	ClearLocalViews();
	if (GetWorld()) { GetWorld()->GetTimerManager().ClearTimer(RetryTimer); }
	if (HasAuthority()) { ClosePortalPair(); }
	Super::EndPlay(Reason);
}
