#include "GamePlay/Combat/Skill/Portal/NF_PortalView.h"
#include "GamePlay/Combat/Skill/Portal/NF_Portal.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "SceneView.h"
#include "UObject/ConstructorHelpers.h"

ANF_PortalView::ANF_PortalView()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("ViewRoot")));
	Surface = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalSurface"));
	Surface->SetupAttachment(RootComponent);
	Surface->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Surface->SetCastShadow(false); Surface->SetReceivesDecals(false);
	Surface->SetOnlyOwnerSee(true);
	// Engine plane lies in XY. Turn its normal toward portal +X; local X becomes portal -Z.
	Surface->SetRelativeRotation(FQuat(FVector::RightVector, HALF_PI));
	Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("PortalCapture"));
	Capture->SetupAttachment(RootComponent);
	Capture->bCaptureEveryFrame = false; Capture->bCaptureOnMovement = false;
	Capture->bAlwaysPersistRenderingState = true;
	Capture->bEnableClipPlane = true;
	Capture->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/ProjectNF/Skills/Portal/M_PortalView.M_PortalView"));
	if (Plane.Succeeded()) { Surface->SetStaticMesh(Plane.Object); }
	if (Material.Succeeded()) { SurfaceMaterial = Material.Object; }
}

void ANF_PortalView::Initialize(ANF_Portal* Portal, APlayerController* Viewer)
{
	Source = Portal; Recipient = Viewer;
	if (Viewer) { SetOwner(Viewer->GetViewTarget()); }
	Surface->SetRelativeScale3D(FVector(FMath::Max(1.0, SurfaceSize.Y) / 100.0, FMath::Max(1.0, SurfaceSize.X) / 100.0, 1.0));
	if (SurfaceMaterial)
	{
		MaterialInstance = UMaterialInstanceDynamic::Create(SurfaceMaterial, this);
		Surface->SetMaterial(0, MaterialInstance);
	}
}

FTransform ANF_PortalView::TransformCamera(const FTransform& Camera, const FTransform& Entrance, const FTransform& Exit)
{
	// Ignore portal mesh/actor scale: crossing a window must not scale camera distance or orientation.
	const FQuat HalfTurn(FVector::UpVector, PI);
	const FVector LocalPosition = Entrance.GetRotation().UnrotateVector(Camera.GetLocation() - Entrance.GetLocation());
	const FQuat LocalRotation = Entrance.GetRotation().Inverse() * Camera.GetRotation();
	return FTransform((Exit.GetRotation() * HalfTurn * LocalRotation).GetNormalized(),
		Exit.GetLocation() + Exit.GetRotation().RotateVector(HalfTurn.RotateVector(LocalPosition)));
}

void ANF_PortalView::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	APlayerController* Viewer = Recipient.Get();
	if (!Source.IsValid() || Source->IsActorBeingDestroyed() || !IsValid(Viewer) || !Viewer->IsLocalController()) { Destroy(); return; }
	ANF_Portal* Exit = Source->GetLinkedPortal();
	const bool bReady = Source->IsPortalVisualVisible() && IsValid(Exit) && Exit->IsPortalVisualVisible() && !Exit->IsActorBeingDestroyed();
	Surface->SetVisibility(bReady && MaterialInstance != nullptr);
	Surface->SetRelativeScale3D(FVector(FMath::Max(1.0, SurfaceSize.Y) / 100.0, FMath::Max(1.0, SurfaceSize.X) / 100.0, 1.0)
		* FMath::Max(Source->GetVisualScaleAlpha(), 0.0001f));
	if (!bReady || !MaterialInstance || !Viewer->PlayerCameraManager || !Viewer->GetLocalPlayer()) { return; }
	// Only this player's view sees this surface, including in split-screen.
	if (GetOwner() != Viewer->GetViewTarget()) { SetOwner(Viewer->GetViewTarget()); }
	const FTransform EntranceFrame = Source->GetViewTransform();
	SetActorLocationAndRotation(EntranceFrame.GetLocation(), EntranceFrame.GetRotation());
	const FMinimalViewInfo& POV = Viewer->PlayerCameraManager->GetCameraCacheView();
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now < NextCaptureAt || FVector::DistSquared(POV.Location, EntranceFrame.GetLocation()) > FMath::Square(MaximumViewDistance)) { return; }
	if (FVector::DotProduct(POV.Rotation.Vector(), EntranceFrame.GetLocation() - POV.Location) < -SurfaceSize.GetMax()) { return; }
	ULocalPlayer* LocalPlayer = Viewer->GetLocalPlayer();
	FSceneViewProjectionData Projection;
	if (!LocalPlayer->ViewportClient || !LocalPlayer->ViewportClient->Viewport
		|| !LocalPlayer->GetProjectionData(LocalPlayer->ViewportClient->Viewport, Projection)) { return; }
	const FIntRect Rect = Projection.GetConstrainedViewRect();
	if (Rect.Width() <= 0 || Rect.Height() <= 0) { return; }
	const double Scale = static_cast<double>(FMath::Clamp(Resolution, 128, 2048)) / FMath::Max(Rect.Width(), Rect.Height());
	const int32 Width = FMath::Max(1, FMath::RoundToInt(Rect.Width() * Scale));
	const int32 Height = FMath::Max(1, FMath::RoundToInt(Rect.Height() * Scale));
	if (!RenderTarget)
	{
		RenderTarget = NewObject<UTextureRenderTarget2D>(this);
		RenderTarget->RenderTargetFormat = RTF_RGBA16f;
		RenderTarget->ClearColor = FLinearColor::Black;
		RenderTarget->InitAutoFormat(Width, Height);
		RenderTarget->UpdateResourceImmediate(true);
		Capture->TextureTarget = RenderTarget;
		MaterialInstance->SetTextureParameterValue(TextureParameter, RenderTarget);
	}
	else if (RenderTarget->SizeX != Width || RenderTarget->SizeY != Height) { RenderTarget->ResizeTarget(Width, Height); }
	const FTransform Camera = TransformCamera(FTransform(POV.Rotation, POV.Location), EntranceFrame, Exit->GetViewTransform());
	Capture->SetWorldLocationAndRotation(Camera.GetLocation(), Camera.GetRotation());
	Capture->FOVAngle = POV.FOV;
	Capture->bUseCustomProjectionMatrix = true;
	Capture->CustomProjectionMatrix = Projection.ProjectionMatrix;
	const FTransform ExitFrame = Exit->GetViewTransform();
	// Clip the half-space between the virtual camera and the destination plane, on either approach side.
	const FVector ExitNormal = ExitFrame.GetUnitAxis(EAxis::X);
	const float Side = FVector::DotProduct(Camera.GetLocation() - ExitFrame.GetLocation(), ExitNormal) >= 0 ? -1.0f : 1.0f;
	Capture->ClipPlaneNormal = ExitNormal * Side;
	Capture->ClipPlaneBase = ExitFrame.GetLocation() + Capture->ClipPlaneNormal * ClipPlaneOffset;
	Capture->HiddenActors.Reset();
	for (TActorIterator<ANF_PortalView> It(GetWorld()); It; ++It) { Capture->HiddenActors.Add(*It); }
	for (TActorIterator<ANF_Portal> It(GetWorld()); It; ++It) { Capture->HiddenActors.Add(*It); }
	Capture->CaptureScene();
	NextCaptureAt = Now + 1.0 / FMath::Clamp(CaptureRate, 1.0f, 60.0f);
}
