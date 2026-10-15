#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/NFGA_SkillBase.h"
#include "NFGA_AirStrike.generated.h"

class ANF_AirStrike;

/** @Owner: 스킬 담당 (구현: Codex) | 2026-10-13
 * 목적: 지면 목표를 선택/확정하고 서버에서 공습 액터를 생성하는 액티브 공격 GA.
 */
UCLASS(Blueprintable)
class PROJECT_NF_API UNFGA_AirStrike : public UNFGA_SkillBase
{
	GENERATED_BODY()
public:
	UNFGA_AirStrike();
	bool CalculateTarget(AActor* Avatar, FVector& OutCenter, FVector& OutFlightDirection) const;
	float GetStrikeRadius() const;
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
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|AirStrike")
	TSubclassOf<ANF_AirStrike> StrikeClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|AirStrike|Targeting", meta = (ClampMin = "100.0", Units = "cm"))
	float MaxTargetDistance = 4000.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|AirStrike|Targeting", meta = (ClampMin = "100.0", Units = "cm"))
	float GroundSearchDepth = 5000.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|AirStrike|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|AirStrike|Damage")
	FGameplayTag DamageMagnitudeTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|AirStrike|Damage", meta = (ClampMin = "0.0"))
	float DamagePerBomb = 30.0f;

private:
	UFUNCTION()
	void StartStrike();
	FVector ConfirmedCenter = FVector::ZeroVector;
	FVector ConfirmedDirection = FVector::ForwardVector;
};
