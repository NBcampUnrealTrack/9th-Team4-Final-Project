#pragma once
#include "CoreMinimal.h"
#include "GameFramework/MovementComponent.h"
#include "NF_GroundTravelComponent.generated.h"

class USphereComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNF_GroundTravelEnded);

// @Owner: 이정준 (구현: Codex) | 2026-10-20
// 서버 전용 지면 추종 이동. 벽/적/낭떠러지/거리 종료를 실행 액터의 폭발 요청으로 연결한다.
UCLASS(ClassGroup = (NF), BlueprintType, meta = (BlueprintSpawnableComponent))
class PROJECT_NF_API UNF_GroundTravelComponent : public UMovementComponent
{
	GENERATED_BODY()
public:
	UNF_GroundTravelComponent();
	bool HasValidSettings() const;
	bool ConfigureComponents();
	bool CalculateStart(AActor* Avatar, FVector AimDirection, float Radius, FVector& Location, FVector& Direction) const;
	bool PrepareTravel(AActor* Avatar, FVector AimDirection);
	bool StartTravel();
	void StopTravel();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	UPROPERTY(BlueprintAssignable, Category = "NF|Ground Travel")
	FNF_GroundTravelEnded OnTravelEnded;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Ground Travel", meta = (ClampMin = "1.0", Units = "cm/s"))
	float TravelSpeed = 1000.0f;
	// @Owner: 이정준 (구현: Codex) | 2026-10-21
	// Starts at TravelSpeed * multiplier, then eases down to TravelSpeed. 1 disables boost.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Ground Travel|Boost", meta = (ClampMin = "1.0"))
	float InitialBoostMultiplier = 2.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Ground Travel|Boost", meta = (ClampMin = "0.0", Units = "s"))
	float BoostDuration = 0.4f;
	UFUNCTION(BlueprintPure, Category = "NF|Ground Travel|Boost")
	float GetTravelSpeedAtTime(float ElapsedSeconds) const;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Ground Travel", meta = (ClampMin = "1.0", Units = "cm"))
	float MaxTravelDistance = 2500.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Ground Travel", meta = (ClampMin = "1.0", Units = "cm"))
	float SpawnForwardDistance = 100.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Ground Travel", meta = (ClampMin = "1.0", Units = "cm"))
	float GroundSearchDepth = 150.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Ground Travel", meta = (ClampMin = "1.0", Units = "cm"))
	float StepHeight = 30.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Ground Travel", meta = (ClampMin = "0.1", Units = "cm"))
	float GroundClearance = 2.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Ground Travel", meta = (ClampMin = "0.1", ClampMax = "0.99"))
	float MinGroundNormalZ = 0.65f;
private:
	friend class FNF_GroundTravelTest;
	float CalculateTravelDistance(float StartTime, float StepSeconds) const;
	bool FindGround(FVector Point, FHitResult& Hit) const;
	void FinishTravel();
	TWeakObjectPtr<USphereComponent> Sphere;
	TWeakObjectPtr<AActor> Caster;
	FVector Heading = FVector::ForwardVector;
	float DistanceTravelled = 0.0f;
	float TravelElapsed = 0.0f;
	bool bPrepared = false;
	bool bTravelling = false;
};
