#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "NF_AreaDamageComponent.generated.h"

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FNF_AreaDamageTargetFilter, AActor*, Target);

/** @Owner: 스킬 담당 (구현: Codex) | 2026-10-13
 * 목적: 서버에서 구형 범위 대상을 찾고 필터/중복/가림 검사 후 공통 피해 경로에 전달한다.
 * 폭발/반복 타이밍은 호출 측이 관리한다. 호출마다 ASC 중복을 제거한다.
 */
UCLASS(ClassGroup = (NF), meta = (BlueprintSpawnableComponent), BlueprintType)
class PROJECT_NF_API UNF_AreaDamageComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNF_AreaDamageComponent();

	// 반환값: Spec을 전달한 ASC 수. 무적/면역/GE 조건으로 실제 피해가 없을 수 있다.
	// SourceAvatar는 자신 제외 판정용이다. 피해의 시전자 정보는 DamageSpec에서 가져온다.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Area Damage")
	int32 SubmitSphereDamage(FVector Center, float Radius, const FGameplayEffectSpecHandle& DamageSpec,
		AActor* SourceAvatar, const FNF_AreaDamageTargetFilter& TargetFilter);

	// 기존 액터의 가림 설정을 전달하는 호환용 API.
	void SetOcclusionPolicy(bool bRequireVisibility, ECollisionChannel Channel);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Area Damage|Targeting")
	TArray<TEnumAsByte<ECollisionChannel>> TargetObjectChannels;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Area Damage|Targeting")
	bool bAffectSource = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Area Damage|Targeting")
	FGameplayTagContainer RequiredTargetTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Area Damage|Targeting")
	FGameplayTagContainer BlockedTargetTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Area Damage|Occlusion")
	bool bRequireLineOfSight = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Area Damage|Occlusion")
	TEnumAsByte<ECollisionChannel> OcclusionTraceChannel = ECC_Visibility;
};
