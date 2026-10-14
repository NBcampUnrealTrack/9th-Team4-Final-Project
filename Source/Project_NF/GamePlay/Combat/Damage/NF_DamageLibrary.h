#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayEffectTypes.h"
#include "Engine/HitResult.h"
#include "NF_DamageLibrary.generated.h"

/** @Owner: 스킬 담당 (구현: Codex) | 2026-10-13
 * 목적: 단일/광역/반사 공격이 공유하는 서버 피해 GE 전달 경로.
 * 최종 피해 계산, 실드, 방어력, 사망과 피격 이벤트는 공통 GE/Attribute 처리 측의 책임이다.
 */
UCLASS()
class PROJECT_NF_API UNF_DamageLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// true는 유효한 대상 ASC에 Spec을 전달했다는 뜻이며 실제 피해/GE 적용 성공을 보장하지 않는다.
	// DamageSpec의 원래 시전자 Context를 유지하고 대상별 적중 정보를 독립적으로 구성한다.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Damage")
	static bool SubmitDamageSpec(AActor* AuthorityActor, AActor* Target,
		const FGameplayEffectSpecHandle& DamageSpec, FVector DamageOrigin, const FHitResult& TargetHit);
};
