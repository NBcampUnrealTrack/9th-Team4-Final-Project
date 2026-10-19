#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NF_PortalView.generated.h"

class ANF_Portal;
class APlayerController;
class USceneCaptureComponent2D;
class UStaticMeshComponent;
class UTextureRenderTarget2D;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/** Local, per-player view of a portal. Never participates in teleport or deployment lifetime. */
UCLASS(Blueprintable, Transient)
class PROJECT_NF_API ANF_PortalView : public AActor
{
	GENERATED_BODY()
public:
	ANF_PortalView();
	void Initialize(ANF_Portal* Portal, APlayerController* Viewer);
	static FTransform TransformCamera(const FTransform& Camera, const FTransform& Entrance, const FTransform& Exit);
	virtual void Tick(float DeltaSeconds) override;
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Portal View")
	TObjectPtr<UStaticMeshComponent> Surface;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Portal View")
	TObjectPtr<USceneCaptureComponent2D> Capture;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Portal View")
	TObjectPtr<UMaterialInterface> SurfaceMaterial;
	// Material must sample PortalTexture using screen/viewport UVs, not mesh UVs.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Portal View")
	FName TextureParameter = TEXT("PortalTexture");
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Portal View", meta = (ClampMin = "128", ClampMax = "2048"))
	int32 Resolution = 1024;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Portal View", meta = (ClampMin = "1", ClampMax = "60"))
	float CaptureRate = 30.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Portal View", meta = (ClampMin = "100", Units = "cm"))
	float MaximumViewDistance = 6000.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Portal View", meta = (ClampMin = "0", Units = "cm"))
	float ClipPlaneOffset = 1.0f;
	// Plane size: width along portal Y, height along portal Z.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Portal View")
	FVector2D SurfaceSize = FVector2D(180.0f, 220.0f);
private:
	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> RenderTarget;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MaterialInstance;
	TWeakObjectPtr<ANF_Portal> Source;
	TWeakObjectPtr<APlayerController> Recipient;
	double NextCaptureAt = 0.0;
};
