#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/HitResult.h"
#include "NF_GroundTargetingComponent.generated.h"

/** @Owner: 스킬 담당 (구현: Codex) | 2026-10-13
 * 목적: 스킬 종류와 독립적으로 조준 방향/사거리로 유효한 지면 목표를 구한다.
 */
UCLASS(ClassGroup = (NF), meta = (BlueprintSpawnableComponent), BlueprintType)
class PROJECT_NF_API UNF_GroundTargetingComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UNF_GroundTargetingComponent();
	UFUNCTION(BlueprintCallable, Category = "NF|Ground Targeting")
	bool FindGroundTarget(AActor* SourceAvatar, float TargetRange, float GroundSearchDepth,
		FHitResult& OutGroundHit, FVector& OutDirection) const;

	// 실제 서버 스킬도 동일 계산을 사용한다. 설정을 컴포넌트에 중복 저장하지 않는다.
	static bool CalculateGroundTarget(AActor* SourceAvatar, float TargetRange, float GroundSearchDepth,
		FHitResult& OutGroundHit, FVector& OutDirection);
	static bool ValidateGroundTarget(AActor* Avatar, FVector Center, float Range, float Depth);
};
