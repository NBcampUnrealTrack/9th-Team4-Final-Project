#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NF_BeamTargetingComponent.generated.h"

class UStaticMesh;
class UMaterialInterface;
class UStaticMeshComponent;

// @Owner: 스킬 담당 (구현: Codex) | 2026-10-14
// 목적: 서버 조준으로 빔의 시작/끝을 계산하고 복제된 결과를 클라이언트에서 표시한다.
UCLASS(ClassGroup = (NF), BlueprintType, meta = (BlueprintSpawnableComponent))
class PROJECT_NF_API UNF_BeamTargetingComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UNF_BeamTargetingComponent();
	bool CalculateBeam(AActor* Avatar, FVector& Start, FVector& End) const;
	void ShowBeam(FVector Start, FVector End);
	void HideBeam();
	float GetHalfWidth() const { return HalfWidth; }
	bool HasValidSettings() const { return FMath::IsFinite(Range) && Range > 0.0f && FMath::IsFinite(HalfWidth) && HalfWidth > 0.0f && !ViewOffset.ContainsNaN(); }
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam", meta = (ClampMin = "1.0"))
	float Range = 2000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam", meta = (ClampMin = "0.1"))
	float HalfWidth = 20.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam")
	FVector ViewOffset = FVector::ZeroVector;
	// 관통 빔은 벽만 Block하는 채널을 지정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
	// 100cm 크기, 로컬 X축 방향의 메시를 기준으로 늘린다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam|Visual")
	TObjectPtr<UStaticMesh> VisualMesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam|Visual")
	TObjectPtr<UMaterialInterface> VisualMaterial;
private:
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> BeamMesh;
};
