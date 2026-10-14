#pragma once

#include "CoreMinimal.h"
#include "NF_SkillPreviewData.generated.h"

/** @Owner: 스킬 담당 (구현: Codex) | 2026-10-13
 * 목적: 프리뷰 BP가 특정 스킬 클래스로 Cast하지 않고 사용하는 공통 표시 데이터.
 * 반경/사거리는 GA 설정에서, 투척 정보는 실제 투척 계산에서 가져온다.
 */
USTRUCT(BlueprintType)
struct PROJECT_NF_API FNF_SkillPreviewData
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite, Category = "NF|Preview")
	float DisplayRadius = 0.0f;
	UPROPERTY(BlueprintReadWrite, Category = "NF|Preview")
	float TargetRange = 4000.0f;
	UPROPERTY(BlueprintReadWrite, Category = "NF|Preview")
	float GroundSearchDepth = 5000.0f;
	UPROPERTY(BlueprintReadWrite, Category = "NF|Preview")
	bool bHasProjectilePath = false;
	UPROPERTY(BlueprintReadWrite, Category = "NF|Preview")
	FVector LaunchLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite, Category = "NF|Preview")
	FVector LaunchVelocity = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite, Category = "NF|Preview")
	float ProjectileRadius = 10.0f;
	UPROPERTY(BlueprintReadWrite, Category = "NF|Preview")
	float GravityZ = -980.0f;
	UPROPERTY(BlueprintReadWrite, Category = "NF|Preview")
	float FlightTime = 3.0f;
};
