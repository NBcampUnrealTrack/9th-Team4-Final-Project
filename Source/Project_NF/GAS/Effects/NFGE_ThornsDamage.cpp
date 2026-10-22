#include "GAS/Effects/NFGE_ThornsDamage.h"
#include "AbilitySystemComponent.h"
#include "GAS/Attributes/NF_MonsterAttributeSet.h"
#include "GAS/Attributes/NF_DeployableHealthAttribute.h"
#include "GAS/Tags/NFTag_Skill.h"
#include "Player/Character/NF_PlayerStatusAttribute.h"
#include "GamePlay/Combat/Boss/NF_BossAttributeSet.h"

void UNFExec_ThornsDamage::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& Parameters,
	FGameplayEffectCustomExecutionOutput& Output) const
{
	const auto* ASC = Parameters.GetTargetAbilitySystemComponent();
	const float Damage = Parameters.GetOwningSpec().GetSetByCallerMagnitude(NFTag_Skill::Data_Damage_Reflected, false, 0);
	if (!ASC || !FMath::IsFinite(Damage) || Damage <= 0) { return; }
	FGameplayAttribute Attribute;
	float Sign = -1;
	if (ASC->GetSet<UNF_PlayerStatusAttribute>()) { Attribute = UNF_PlayerStatusAttribute::GetHealthAttribute(); }
	else if (ASC->GetSet<UNF_BossAttributeSet>()) { Attribute = UNF_BossAttributeSet::GetHealthAttribute(); }
	else if (ASC->GetSet<UNF_MonsterAttributeSet>()) { Attribute = UNF_MonsterAttributeSet::GetIncomingDamageAttribute(); Sign = 1; }
	else if (ASC->GetSet<UNF_DeployableHealthAttribute>()) { Attribute = UNF_DeployableHealthAttribute::GetIncomingDamageAttribute(); Sign = 1; }
	if (Attribute.IsValid()) { Output.AddOutputModifier(FGameplayModifierEvaluatedData(Attribute, EGameplayModOp::AddBase, Damage * Sign)); }
}

UNFGE_ThornsDamage::UNFGE_ThornsDamage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	Executions.AddDefaulted_GetRef().CalculationClass = UNFExec_ThornsDamage::StaticClass();
}
