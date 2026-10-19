#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NF_SilhouetteView.generated.h"

class ACharacter;
class APlayerController;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class USkeletalMeshComponent;

USTRUCT(BlueprintType)
struct PROJECT_NF_API FNF_SilhouetteStyle
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Silhouette")
	FLinearColor Color = FLinearColor(0.0f, 0.8f, 1.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Silhouette", meta = (ClampMin = "0", ClampMax = "1"))
	float FillOpacity = 0.45f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Silhouette", meta = (ClampMin = "0", ClampMax = "1"))
	float OutlineOpacity = 0.9f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Silhouette", meta = (ClampMin = "0", ClampMax = "4", ToolTip = "Outline width in pixels."))
	float OutlineWidth = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Silhouette", meta = (ClampMin = "0", Units = "cm"))
	float DepthBias = 3.0f;
	bool IsValid() const;
};

USTRUCT()
struct FNF_SilhouetteMesh
{
	GENERATED_BODY()
	UPROPERTY()
	TWeakObjectPtr<USkeletalMeshComponent> Source;
	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> Proxy;
};

/** One local presentation manager per player. Never edits the target's materials/depth/stencil settings. */
UCLASS(NotBlueprintable, Transient)
class PROJECT_NF_API ANF_SilhouetteView : public AActor
{
	GENERATED_BODY()
public:
	ANF_SilhouetteView();
	static ANF_SilhouetteView* FindOrCreate(APlayerController* Recipient);
	void UpdateRequest(FGuid SourceId, const TArray<TObjectPtr<ACharacter>>& Targets, double ExpiresAt,
		UMaterialInterface* Material, const FNF_SilhouetteStyle& Style);
	void RemoveRequest(FGuid SourceId);
	virtual void Tick(float DeltaSeconds) override;
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	friend class FNF_SonarViewTest;
	struct FRequest
	{
		TArray<TWeakObjectPtr<ACharacter>> Targets;
		TWeakObjectPtr<UMaterialInterface> Material;
		FNF_SilhouetteStyle Style;
		double ExpiresAt = 0.0;
		uint64 Order = 0;
	};
	void ReconcileMeshes();
	void ClearMeshes();
	TMap<FGuid, FRequest> Requests;
	uint64 NextOrder = 1;
	int32 StencilValue = 240;
	UPROPERTY(Transient)
	TArray<FNF_SilhouetteMesh> Meshes;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MaterialInstance;
};
