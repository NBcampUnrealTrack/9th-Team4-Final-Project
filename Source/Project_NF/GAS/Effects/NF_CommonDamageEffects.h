#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GameplayEffectExecutionCalculation.h"
#include "NF_CommonDamageEffects.generated.h"

// 방어력 계산, 실드먼저 감소
UCLASS()
class PROJECT_NF_API UNFExecCalc_CommonDamage
	: public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
	
public:
	UNFExecCalc_CommonDamage();
	
protected:
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};

// 실제 피해 적용할 때 GE
UCLASS()
class PROJECT_NF_API UNFGE_CommonDamage : public UGameplayEffect
{
	GENERATED_BODY()
	
public:
	UNFGE_CommonDamage();
};