#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MotionMatchingTraversableBlock.generated.h"

class USplineComponent;

namespace MotionMatchingCollision
{
	constexpr ECollisionChannel Traversable = ECC_GameTraceChannel1;
}

USTRUCT(BlueprintType)
struct FMotionMatchingLedgeResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	bool bIsFrontLedgeFound = false;

	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	FVector FrontLedgeLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	FVector FrontLedgeNormal = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	bool bIsBackLedgeFound = false;

	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	FVector BackLedgeLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	FVector BackLedgeNormal = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FMotionMatchingLedgePair
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	FName LedgeName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	FName OppositeLedgeName;
};

UCLASS(Blueprintable)
class PROJECT_NF_API AMotionMatchingTraversableBlock : public AActor
{
	GENERATED_BODY()

public:
	AMotionMatchingTraversableBlock();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;

	UFUNCTION(BlueprintCallable, Category = "Traversal")
	FMotionMatchingLedgeResult ComputeLedgeTransforms(const FVector& HitLocation, const FVector& ActorLocation) const;

	UFUNCTION(BlueprintCallable, Category = "Traversal")
	USplineComponent* FindLedgeClosestToActor(const FVector& ActorLocation) const;

	UFUNCTION(BlueprintCallable, Category = "Traversal")
	FVector GetLedgeNormal(const USplineComponent* Ledge, const FVector& WorldLocation) const;

	UFUNCTION(BlueprintCallable, Category = "Traversal")
	void RefreshLedges();

#if WITH_EDITOR
	UFUNCTION(CallInEditor, Category = "Traversal|Debug")
	void DebugDrawLedges();
#endif

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal", meta = (ClampMin = "0"))
	float MinLedgeWidth = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	FName LedgeComponentTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	bool bIsAutoPairOppositeLedges = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal", meta = (EditCondition = "bIsAutoPairOppositeLedges == false"))
	TArray<FMotionMatchingLedgePair> ManualLedgePairs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	bool bIsSplineUpVectorUsedAsNormal = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	bool bIsTraversableCollisionAutoSet = true;

	// [추가] D-1: 켜면 메시 경계 상자 윗면의 네 모서리에 스플라인(턱) 4개를 자동으로 맞춘다.
	// 액터나 메시의 스케일·회전을 바꿔도 턱이 모서리를 따라감. 직접 찍은 스플라인을 쓰려면 끈다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	bool bIsLedgeAutoFitted = true;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Traversal")
	TArray<TObjectPtr<USplineComponent>> LedgeSplines;

	UPROPERTY(Transient)
	TMap<TObjectPtr<USplineComponent>, TObjectPtr<USplineComponent>> OppositeLedgeMap;

private:
	void CollectLedges();
	void FitLedgesToMeshBounds(); // [추가] D-1
	void PairLedges();
	void PairLedgesManually();
	void PairLedgesAutomatically();
	USplineComponent* FindOppositeLedgeAutomatically(USplineComponent* Ledge) const;
	USplineComponent* FindLedgeByName(const FName& LedgeName) const;
	void ApplyTraversableCollision();
	void CacheObstacleCenter();
	FVector GetObstacleCenter() const;
	FVector GetLedgeMiddleLocation(const USplineComponent* Ledge) const;

	FVector CachedLocalCenter = FVector::ZeroVector;
};
