#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GameplayEffectExecutionCalculation.h"
#include "NF_BossCombatEffects.generated.h"

UCLASS()
class PROJECT_NF_API UNFExecCalc_BossDamageToPlayer : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

protected:
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};

UCLASS()
class PROJECT_NF_API UNFExecCalc_PlayerDamageToBoss : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

protected:
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};

UCLASS()
class PROJECT_NF_API UNFGE_BossDamageToPlayer : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UNFGE_BossDamageToPlayer();
};

UCLASS()
class PROJECT_NF_API UNFGE_PlayerDamageToBoss : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UNFGE_PlayerDamageToBoss();
};

UCLASS()
class PROJECT_NF_API UNFGE_BossStun : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UNFGE_BossStun();
};

UCLASS()
class PROJECT_NF_API UNFGE_BossAirborne : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UNFGE_BossAirborne();
};
