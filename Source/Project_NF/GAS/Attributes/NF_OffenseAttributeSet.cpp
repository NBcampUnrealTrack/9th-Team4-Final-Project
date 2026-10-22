#include "GAS/Attributes/NF_OffenseAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"

UNF_OffenseAttributeSet::UNF_OffenseAttributeSet()
{
	InitAttackDamageBonus(0.0f); InitLastRoundDamageMultiplier(1.0f);
}
void UNF_OffenseAttributeSet::ClampValue(const FGameplayAttribute& Attribute, float& Value) const
{
	const float Minimum = Attribute == GetLastRoundDamageMultiplierAttribute() ? 1.0f : 0.0f;
	Value = FMath::IsFinite(Value) ? FMath::Max(Value, Minimum) : Minimum;
}
void UNF_OffenseAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue); ClampValue(Attribute, NewValue);
}
void UNF_OffenseAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue); ClampValue(Attribute, NewValue);
}
void UNF_OffenseAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UNF_OffenseAttributeSet, AttackDamageBonus, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNF_OffenseAttributeSet, LastRoundDamageMultiplier, COND_None, REPNOTIFY_Always);
}
void UNF_OffenseAttributeSet::OnRep_AttackDamageBonus(const FGameplayAttributeData& OldValue)
{
	if (auto* ASC = FindRegisteredASC()) { ASC->SetBaseAttributeValueFromReplication(GetAttackDamageBonusAttribute(), AttackDamageBonus, OldValue); }
}
void UNF_OffenseAttributeSet::OnRep_LastRoundDamageMultiplier(const FGameplayAttributeData& OldValue)
{
	if (auto* ASC = FindRegisteredASC()) { ASC->SetBaseAttributeValueFromReplication(GetLastRoundDamageMultiplierAttribute(), LastRoundDamageMultiplier, OldValue); }
}

UAbilitySystemComponent* UNF_OffenseAttributeSet::FindRegisteredASC() const
{
	// AttributeSet's default actor lookup can resolve a different ASC when NF_SkillTest coexists.
	auto* Primary = GetOwningAbilitySystemComponent();
	if (Primary && Primary->GetSet<UNF_OffenseAttributeSet>() == this) { return Primary; }
	TArray<UAbilitySystemComponent*> Components; GetOwningActor()->GetComponents(Components);
	for (auto* ASC : Components) { if (ASC->GetSet<UNF_OffenseAttributeSet>() == this) { return ASC; } }
	return nullptr;
}
