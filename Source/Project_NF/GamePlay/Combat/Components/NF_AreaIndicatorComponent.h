#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NF_AreaIndicatorComponent.generated.h"

class UDecalComponent;
class UMaterialInterface;

/** @Owner: 스킬 담당 (구현: Codex) | 2026-10-12
 * 목적: 위치, 반경, 표면 법선을 받아 원형 범위를 표시하는 재사용 컴포넌트.
 * 수류탄/GA/GE에 의존하지 않으며 피해 판정과 네트워크 전달은 호출 측이 담당한다.
 */
UCLASS(ClassGroup = (NF), meta = (BlueprintSpawnableComponent), BlueprintType)
class PROJECT_NF_API UNF_AreaIndicatorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNF_AreaIndicatorComponent();

	UFUNCTION(BlueprintCallable, Category = "NF|Area Indicator")
	void ShowArea(FVector Location, float Radius, FVector SurfaceNormal);

	UFUNCTION(BlueprintCallable, Category = "NF|Area Indicator")
	void HideArea();

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 원형 마스크를 가진 Deferred Decal 머티리얼. 비워두면 테스트용 원형 디버그 표시.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Area Indicator")
	TObjectPtr<UMaterialInterface> DecalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Area Indicator", meta = (ClampMin = "1.0"))
	float ProjectionDepth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Area Indicator", meta = (ClampMin = "0.0"))
	float SurfaceOffset = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Area Indicator|Debug")
	FColor AreaColor = FColor(255, 160, 30, 80);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Area Indicator|Debug")
	FColor BorderColor = FColor::Orange;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Area Indicator|Debug", meta = (ClampMin = "12", ClampMax = "128"))
	int32 CircleSegments = 64;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Area Indicator|Debug", meta = (ClampMin = "0.1"))
	float BorderThickness = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Area Indicator|Debug")
	bool bDrawFilledArea = true;

private:
	void UpdateDecal();
	void DrawDebugArea() const;

	UPROPERTY(Transient)
	TObjectPtr<UDecalComponent> AreaDecal;

	FVector AreaLocation = FVector::ZeroVector;
	FVector AreaNormal = FVector::UpVector;
	float AreaRadius = 0.0f;
	bool bAreaVisible = false;
};
