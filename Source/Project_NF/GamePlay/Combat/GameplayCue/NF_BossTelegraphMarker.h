#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NF_BossTelegraphMarker.generated.h"

class UDecalComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/** Local-only decal instance created for one replicated boss GameplayCue. */
UCLASS(NotBlueprintable, NotPlaceable, Transient)
class PROJECT_NF_API ANF_BossTelegraphMarker : public AActor
{
	GENERATED_BODY()

public:
	ANF_BossTelegraphMarker();
	void InitializeMarker(const FVector& Location, float Radius, float Duration,
		UMaterialInterface* Material, FName ProgressParameterName,
		float ProjectionDepth, float HeightOffset);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UDecalComponent> TelegraphDecal;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> MarkerMaterial;

	FVector MarkerLocation = FVector::ZeroVector;
	float MarkerRadius = 0.0f;
	float MarkerDuration = 2.0f;
	float MarkerProjectionDepth = 100.0f;
	float MarkerHeightOffset = 2.0f;
	float ElapsedTime = 0.0f;
	FName ProgressParameterName = TEXT("Progress");
};
