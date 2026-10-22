#include "NF_CommonDamageEffects.h"
#include "GAS/Tags/NFTag_BossCombat.h"
#include "Player/Character/NF_PlayerStatusAttribute.h"
#include "GAS/Attributes/NF_KeyHolderAttributeSet.h"

namespace 
{
	// 데미지를 처리할 수 있는 값인지 확인용 헬퍼 함수
	bool ShouldProcessDamage(float Damage, bool bAllowZeroDamage = false)
	{
		// 정상적인 숫자가 아니거나 음수x
		if (!FMath::IsFinite(Damage) || Damage < 0.0f)
		{
			return false;
		}
		
		// 0뎀을 허용하지 않는 경우 처리x
		if (!bAllowZeroDamage && Damage == 0.0f)
		{
			return false;
		}
		
		return true;
	}
	
	struct FCommonDamageCapture
	{
		DECLARE_ATTRIBUTE_CAPTUREDEF(Defence);
		DECLARE_ATTRIBUTE_CAPTUREDEF(Shield);
		DECLARE_ATTRIBUTE_CAPTUREDEF(DamageReductionRate);
		
		FCommonDamageCapture()
		{
			// 방어력 가져오기
			DefenceProperty = UNF_PlayerStatusAttribute::GetDefenceAttribute().GetUProperty();
			DefenceDef = FGameplayEffectAttributeCaptureDefinition(
				UNF_PlayerStatusAttribute::GetDefenceAttribute(),
				EGameplayEffectAttributeCaptureSource::Target,
				false);
			
			// 실드 가져오기
			ShieldProperty = UNF_PlayerStatusAttribute::GetShieldAttribute().GetUProperty();
			ShieldDef = FGameplayEffectAttributeCaptureDefinition(
				UNF_PlayerStatusAttribute::GetShieldAttribute(),
				EGameplayEffectAttributeCaptureSource::Target,
				false);
			
			// 열쇠 보유자의 받는 피해 감소 가져오기
			DamageReductionRateProperty =
				UNF_KeyHolderAttributeSet::GetDamageReductionRateAttribute().GetUProperty();
			DamageReductionRateDef = FGameplayEffectAttributeCaptureDefinition(
				UNF_KeyHolderAttributeSet::GetDamageReductionRateAttribute(),
				EGameplayEffectAttributeCaptureSource::Target,
				false);
		}
	};
	
	const FCommonDamageCapture& GetCommonDamageCapture()
	{
		static FCommonDamageCapture Capture;
		return Capture;
	}
}

UNFExecCalc_CommonDamage::UNFExecCalc_CommonDamage()
{
	// 데미지 계산에 필요한 방어력, 실드 정보
	RelevantAttributesToCapture.Add(
		GetCommonDamageCapture().DefenceDef);
	
	RelevantAttributesToCapture.Add(
		GetCommonDamageCapture().ShieldDef);
	
	// 열쇠 보유 효과 받피감
	RelevantAttributesToCapture.Add(
		GetCommonDamageCapture().DamageReductionRateDef);
}

void UNFExecCalc_CommonDamage::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	
	// 공격 측 SetByCaller로 전달한 데미지 값 가져옴
	const float AttackDamage = Spec.GetSetByCallerMagnitude(
		NFTag_BossCombat::Data_Damage, false, 0.0f);
	
	// 데미지가 처리 가능한지
	if (!ShouldProcessDamage(AttackDamage))
	{
		return;
	}
	
	FAggregatorEvaluateParameters EvaluateParams;
	
	EvaluateParams.SourceTags = 
		Spec.CapturedSourceTags.GetAggregatedTags();
	
	EvaluateParams.TargetTags =
		Spec.CapturedTargetTags.GetAggregatedTags();
	
	float Defence = 0.0f;
	float Shield = 0.0f;
	float DamageReductionRate = 0.0f;
	
	// 피해를 받는 플레이어의 현재 방어력 가져옴
	const bool bHasDefence =
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
			GetCommonDamageCapture().DefenceDef,
			EvaluateParams,
			Defence);
	
	// 피해를 받는 플레이어의 현재 실드를 가져옴
	const bool bHasShield =
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
			GetCommonDamageCapture().ShieldDef,
			EvaluateParams,
			Shield);
	
	// 받피감으로 얼마나 감소하는지
	const bool bHasDamageReduction =
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
			GetCommonDamageCapture().DamageReductionRateDef,
			EvaluateParams,
			DamageReductionRate);
	
	// 방어력 or 실드 못 가져오면 피해x
	if (!bHasDefence || !bHasShield)
	{
		return;
	}
	
	// 방어력이나 실드 값이 올바르지 않으면 처리x
	if (!FMath::IsFinite(Defence) || !FMath::IsFinite(Shield))
	{
		return;
	}
	
	Defence = FMath::Max(Defence, 0.0f);
	Shield = FMath::Max(Shield, 0.0f);
	
	// 받피감 가져오지 못하면 받피감 없이 계산
	if (!bHasDamageReduction || !FMath::IsFinite(DamageReductionRate))
	{
		DamageReductionRate = 0.0f;
	}
	
	DamageReductionRate = FMath::Clamp(DamageReductionRate, 0.0f, 1.0f);
	
	// 데미지에서 방어력을 빼서 실제 데미지 계산
	const float DamageAfterDefence =
		FMath::Max(AttackDamage - Defence, 0.0f);
	
	// 받피감 적용
	const float FinalDamage =
		DamageAfterDefence * (1.0f - DamageReductionRate);
	
	// 데미지가 방어력을 뚫지 못하면 종료
	if (FinalDamage <= 0.0f)
	{
		return;
	}
	
	// 현재 실드로 막을 수 있는 데미지 계산
	const float ShieldDamage =
		FMath::Min(FinalDamage, Shield);
	
	// 실드로 막지 못한 나머지 데미지 계산
	const float HealthDamage = FinalDamage - ShieldDamage;
	
	// 실드가 남아 있으면 실드먼저
	if (ShieldDamage > 0.0f)
	{
		OutExecutionOutput.AddOutputModifier(
			FGameplayModifierEvaluatedData(
				UNF_PlayerStatusAttribute::GetShieldAttribute(),
				EGameplayModOp::Additive,
				-ShieldDamage));
	}
	
	// 실드로 막지 못한 데미지는 체력에서 감소
	if (HealthDamage > 0.0f)
	{
		OutExecutionOutput.AddOutputModifier(
			FGameplayModifierEvaluatedData(
				UNF_PlayerStatusAttribute::GetHealthAttribute(),
				EGameplayModOp::Additive,
				-HealthDamage));
	}
}

UNFGE_CommonDamage::UNFGE_CommonDamage()
{
	// 피해 즉시 적용 GE
	DurationPolicy = EGameplayEffectDurationType::Instant;
	
	// GE가 적용될 때 공통 피해 계산 함수 실행
	Executions.AddDefaulted_GetRef().CalculationClass = 
		UNFExecCalc_CommonDamage::StaticClass();
}