#pragma once

#include "CoreMinimal.h"
#include "UI/Minimap/NF_MinimapTypes.h"
#include "NF_MinimapSnapshot.generated.h"


USTRUCT(BlueprintType)
struct PROJECT_NF_API FNF_MinimapSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Minimap")
	bool bHasView = false;

	UPROPERTY(BlueprintReadOnly, Category = "Minimap")
	FVector ViewLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Minimap")
	float ViewYaw = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Minimap")
	TArray<FNF_MinimapMarker> Markers;
};