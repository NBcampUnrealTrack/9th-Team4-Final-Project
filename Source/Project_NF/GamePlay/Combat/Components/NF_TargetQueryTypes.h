#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/HitResult.h"
#include "NF_TargetQueryTypes.generated.h"

class UWorld;
struct FCollisionShape;
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FNF_AreaDamageTargetFilter, AActor*, Target);

// @Owner: 스킬 담당 (구현: Codex) | 2026-10-14
// 목적: 피해와 탐색이 공유하는 범위 모양/필터/결과. 기존 범위 API의 타입 이름을 유지한다.
UENUM(BlueprintType)
enum class ENF_AreaDamageShape : uint8 { Sphere, Box, Capsule, Sector };
USTRUCT(BlueprintType)
struct PROJECT_NF_API FNF_AreaDamageShape
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape")
	ENF_AreaDamageShape Shape = ENF_AreaDamageShape::Sphere;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape", meta = (ClampMin = "1.0"))
	float Radius = 300.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape")
	FVector BoxHalfExtent = FVector(300.0f, 100.0f, 100.0f);
	// 반구를 포함한 로컬 Z축 절반 높이.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape", meta = (ClampMin = "1.0"))
	float CapsuleHalfHeight = 300.0f;
	// 로컬 X축 전방. 충돌 컴포넌트 Bounds 중심을 검사한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float SectorHalfAngle = 45.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape", meta = (ClampMin = "1.0"))
	float SectorHalfHeight = 100.0f;
};

UENUM(BlueprintType)
enum class ENF_TargetTeamFilter : uint8 { All, Allies, Enemies };
USTRUCT(BlueprintType)
struct PROJECT_NF_API FNF_TargetFilterSettings
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter")
	bool bIncludeSource = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter")
	bool bRequireAbilitySystem = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter")
	ENF_TargetTeamFilter Team = ENF_TargetTeamFilter::All;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter")
	FGameplayTagContainer RequiredTags;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter")
	FGameplayTagContainer BlockedTags;
};

USTRUCT(BlueprintType)
struct PROJECT_NF_API FNF_TargetQuerySettings
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
	FNF_AreaDamageShape Shape;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
	FNF_TargetFilterSettings Filter;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
	TArray<TEnumAsByte<ECollisionChannel>> ObjectChannels = { ECC_Pawn, ECC_WorldDynamic, ECC_PhysicsBody };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
	bool bRequireLineOfSight = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
	bool bSortByDistance = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query", meta = (ClampMin = "0"))
	int32 MaxTargets = 0;
	// 피해 적용은 ASC별 중복을 제거한다. 일반 탐색은 액터별 결과를 반환할 수 있다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
	bool bDeduplicateAbilitySystems = false;
};

USTRUCT(BlueprintType)
struct PROJECT_NF_API FNF_TargetQueryResult
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	TObjectPtr<AActor> Target = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	FHitResult Hit;
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	float Distance = 0.0f;
};

PROJECT_NF_API bool NF_BuildQueryShape(const FNF_AreaDamageShape& Settings, FCollisionShape& OutShape);
