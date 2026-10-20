#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "TimerManager.h"
#include "NFGA_MonsterLeapAttack.generated.h"

class ANF_Monster;

UCLASS()
class PROJECT_NF_API UNFGA_MonsterLeapAttack : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UNFGA_MonsterLeapAttack();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	UFUNCTION()
	void HandlePreparationFinished();

	UFUNCTION()
	void HandleMonsterLanded(const FHitResult& Hit);

	void HandleLandingTimeout();
	
	UFUNCTION()
	void HandleRecoveryFinished();

	void FinishLeapAbility(bool bWasCancelled);

	// 도약하기 전 준비 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "NF|Monster|Leap",
		meta = (ClampMin = "0.01"))
	float PreparationTime = 0.7f;

	// 평지 기준으로 목표 지점까지 도달하는 데 사용할 예상 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "NF|Monster|Leap",
		meta = (ClampMin = "0.2"))
	float FlightTime = 1.0f;

	// 도약 목표 지점까지의 최대 수평 거리
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "NF|Monster|Leap",
		meta = (ClampMin = "0.0"))
	float MaxLeapDistance = 800.0f;

	// 착지 이벤트가 오지 않을 경우 Ability를 종료하는 안전 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "NF|Monster|Leap",
		meta = (ClampMin = "0.1"))
	float LandingTimeout = 3.0f;
	
	// 착지 충격파의 반경
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "NF|Monster|Leap",
		meta = (ClampMin = "0.0", Units = "cm"))
	float LandingDamageRadius = 300.0f;

	// AttackPower에 곱할 착지 공격의 피해 배율
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "NF|Monster|Leap",
		meta = (ClampMin = "0.0"))
	float LandingDamageMultiplier = 1.5f;
	
	// 착지 후딜
	UPROPERTY(
	EditDefaultsOnly,
	BlueprintReadOnly,
	Category = "NF|Monster|Attack",
	meta = (ClampMin = "0.0", Units = "s"))
	float RecoveryDuration = 0.8f;

private:
	TWeakObjectPtr<ANF_Monster> LeapingMonster;

	FTimerHandle LandingTimeoutHandle;
};
