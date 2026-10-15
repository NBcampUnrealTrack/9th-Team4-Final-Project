#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "NFGA_MonsterRangedAttack.generated.h"

class ANF_MonsterProjectile;
class UGameplayEffect;

UCLASS()
class PROJECT_NF_API UNFGA_MonsterRangedAttack : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UNFGA_MonsterRangedAttack();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	UFUNCTION()
	void HandleAttackDelayFinished();

	UPROPERTY(EditDefaultsOnly, Category = "NF|Monster|RangedAttack")
	TSubclassOf<ANF_MonsterProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "NF|Monster|RangedAttack")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditDefaultsOnly, Category = "NF|Monster|RangedAttack",
		meta = (ClampMin = "0.0"))
	float AttackInterval = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "NF|Monster|RangedAttack",
		meta = (ClampMin = "0.0"))
	float ProjectileSpawnForwardOffset = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "NF|Monster|RangedAttack")
	FVector ProjectileSpawnOffset = FVector(0.0f, 0.0f, 80.0f);
};
