#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/Active/NFGA_ActiveSkillBase.h"
#include "NFGA_Grenade.generated.h"

class ANF_Grenade;

/** @Owner: 스킬 담당 (구현: Codex) | 2026-10-12
 * 목적: 사용 검사와 준비 시간 처리 후 서버에서 수류탄을 투척한다.
 * 자식 BP는 설정값을 지정한다. 실행 흐름은 이 C++ GA가 담당한다.
 */
UCLASS(Blueprintable)
class PROJECT_NF_API UNFGA_Grenade : public UNFGA_ActiveSkillBase
{
	GENERATED_BODY()

public:
	UNFGA_Grenade();

	// 동일한 기본값을 가진 GA의 로컬 표시와 서버 투척에서 공유한다.
	bool CalculateThrowParameters(AActor* Avatar, FVector& OutSpawnLocation, FVector& OutVelocity) const;
	TSubclassOf<ANF_Grenade> GetGrenadeClass() const { return GrenadeClass; }
	virtual bool GetSkillPreviewData_Implementation(AActor* SourceAvatar, FNF_SkillPreviewData& OutData) const override;
	virtual bool BuildConfirmedTargetData_Implementation(FGameplayAbilityTargetDataHandle& OutTargetData) const override;
	virtual bool ValidateConfirmedTargetData_Implementation(const FGameplayAbilityTargetDataHandle& TargetData) const override;

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Grenade")
	TSubclassOf<ANF_Grenade> GrenadeClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Grenade|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// 공통 피해 GE가 읽는 SetByCaller 태그와 동일하게 지정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Grenade|Damage")
	FGameplayTag DamageMagnitudeTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Grenade|Damage", meta = (ClampMin = "0.0"))
	float Damage = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Grenade|Throw", meta = (ClampMin = "1.0", Units = "cm/s"))
	float ThrowSpeed = 1200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Grenade|Throw", meta = (ClampMin = "-80.0", ClampMax = "80.0", Units = "deg"))
	float ThrowAngleOffset = 15.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Grenade|Throw", meta = (ClampMin = "1.0", Units = "cm"))
	float AimDistance = 10000.0f;

	// 손 소켓이 없을 때 사용하는 눈 위치 기준 전방/위쪽 생성 오프셋.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Grenade|Throw")
	FVector SpawnOffset = FVector(60.0f, 0.0f, -20.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Grenade|Throw")
	FName ThrowSocketName = NAME_None;

private:
	bool CalculateThrowParametersForDirection(AActor* Avatar, const FVector& AimDirection, FVector& OutSpawnLocation, FVector& OutVelocity) const;
	FVector ConfirmedAimDirection = FVector::ForwardVector;
	UFUNCTION()
	void ThrowGrenade();
};
