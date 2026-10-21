#pragma once
#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GameplayEffectExecutionCalculation.h"
#include "NFGE_ThornsDamage.generated.h"

// @Owner: 이정준 (구현: Codex) | 2026-10-22
UCLASS()
class PROJECT_NF_API UNFExec_ThornsDamage : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
public:
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& Parameters,
		FGameplayEffectCustomExecutionOutput& Output) const override;
};

UCLASS(Blueprintable)
class PROJECT_NF_API UNFGE_ThornsDamage : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UNFGE_ThornsDamage();
};
