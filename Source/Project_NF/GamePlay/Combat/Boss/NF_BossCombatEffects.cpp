#include "GamePlay/Combat/Boss/NF_BossCombatEffects.h"

#include "GAS/Tags/NFTag_BossCombat.h"
#include "Player/Character/NF_PlayerStatusAttribute.h"
#include "GamePlay/Combat/Boss/NF_BossAttributeSet.h"

void UNFExecCalc_BossDamageToPlayer::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const float Damage = FMath::Max(ExecutionParams.GetOwningSpec().GetSetByCallerMagnitude(
		NFTag_BossCombat::Data_Damage, false, 0.0f), 0.0f);
	if (Damage > 0.0f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			UNF_PlayerStatusAttribute::GetHealthAttribute(), EGameplayModOp::Additive, -Damage));
	}
}

void UNFExecCalc_PlayerDamageToBoss::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const float Damage = FMath::Max(ExecutionParams.GetOwningSpec().GetSetByCallerMagnitude(
		NFTag_BossCombat::Data_Damage, false, 0.0f), 0.0f);
	if (Damage > 0.0f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			UNF_BossAttributeSet::GetHealthAttribute(), EGameplayModOp::Additive, -Damage));
	}
}

UNFGE_BossDamageToPlayer::UNFGE_BossDamageToPlayer()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	Executions.AddDefaulted_GetRef().CalculationClass = UNFExecCalc_BossDamageToPlayer::StaticClass();
}

UNFGE_PlayerDamageToBoss::UNFGE_PlayerDamageToBoss()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	Executions.AddDefaulted_GetRef().CalculationClass = UNFExecCalc_PlayerDamageToBoss::StaticClass();
}

UNFGE_BossStun::UNFGE_BossStun()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FScalableFloat(1.0f);
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	InheritableOwnedTagsContainer.AddTag(NFTag_BossCombat::State_Stunned);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

UNFGE_BossAirborne::UNFGE_BossAirborne()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FScalableFloat(2.0f);
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	InheritableOwnedTagsContainer.AddTag(NFTag_BossCombat::State_Airborne);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
}
