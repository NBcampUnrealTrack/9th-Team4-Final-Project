#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/HitResult.h"
#include "NF_ProjectileTrajectoryComponent.generated.h"

/** @Owner: 스킬 담당 (구현: Codex) | 2026-10-12
 * 목적: 로컬 조준 중 전달받은 투척 값으로 첫 충돌까지의 예상 궤적을 표시한다.
 * 피해/발동 로직 없이 다른 곡사 투사체에도 재사용하는 테스트용 표시 컴포넌트.
 */
UCLASS(ClassGroup = (NF), meta = (BlueprintSpawnableComponent), BlueprintType)
class PROJECT_NF_API UNF_ProjectileTrajectoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNF_ProjectileTrajectoryComponent();

	UFUNCTION(BlueprintCallable, Category = "NF|Trajectory")
	void StartPreview();

	UFUNCTION(BlueprintCallable, Category = "NF|Trajectory")
	void StopPreview();

	// 조준 중 매 프레임 호출한다. 표시 선은 한 프레임만 유지되므로 중단 시 사라진다.
	UFUNCTION(BlueprintCallable, Category = "NF|Trajectory")
	void UpdatePreview(FVector StartLocation, FVector LaunchVelocity, float ProjectileRadius,
		float GravityZ, float FlightTime);

	// 그 프레임의 첫 충돌 정보를 제공한다. 충돌 예측이 없거나 표시 종료 시 false.
	UFUNCTION(BlueprintPure, Category = "NF|Trajectory")
	bool GetPredictedImpact(FHitResult& OutHit) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Trajectory", meta = (ClampMin = "10.0", ClampMax = "60.0"))
	float SimulationFrequency = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Trajectory")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_WorldDynamic;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Trajectory")
	FColor PathColor = FColor::Green;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Trajectory")
	FColor ImpactColor = FColor::Orange;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Trajectory", meta = (ClampMin = "0.1"))
	float LineThickness = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Trajectory", meta = (ClampMin = "1.0"))
	float ImpactMarkerRadius = 20.0f;

private:
	bool bPreviewEnabled = false;
	bool bHasPredictedImpact = false;
	FHitResult PredictedImpact;
};
