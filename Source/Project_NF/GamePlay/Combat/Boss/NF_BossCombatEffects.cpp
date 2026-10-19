#include "GamePlay/Combat/Boss/NF_BossCombatEffects.h"

#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
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
		const UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
		const UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
		const AActor* SourceActor = SourceASC ? SourceASC->GetAvatarActor() : nullptr;
		const AActor* TargetActor = TargetASC ? TargetASC->GetAvatarActor() : nullptr;

#if WITH_EDITOR
		if (GEngine)
		{
			const FString DamageMessage = FString::Printf(TEXT("Boss Damage | %s -> %s | %.2f"),
				*GetNameSafe(SourceActor), *GetNameSafe(TargetActor), Damage);
			const int32 MessageKey = TargetActor ? static_cast<int32>(TargetActor->GetUniqueID()) : INDEX_NONE;
			GEngine->AddOnScreenDebugMessage(MessageKey, 2.0f, FColor::Red, DamageMessage);
		}
#endif

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
