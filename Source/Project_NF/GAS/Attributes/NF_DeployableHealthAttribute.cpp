#include "GAS/Attributes/NF_DeployableHealthAttribute.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UNF_DeployableHealthAttribute::UNF_DeployableHealthAttribute()
{
	InitMaxHealth(100.0f);
	InitHealth(100.0f);
	InitIncomingDamage(0.0f);
}

void UNF_DeployableHealthAttribute::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UNF_DeployableHealthAttribute, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UNF_DeployableHealthAttribute, MaxHealth, COND_None, REPNOTIFY_Always);
}

void UNF_DeployableHealthAttribute::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (!FMath::IsFinite(NewValue)) { NewValue = Attribute == GetMaxHealthAttribute() ? 1.0f : 0.0f; }
	if (Attribute == GetMaxHealthAttribute()) { NewValue = FMath::Max(NewValue, 1.0f); }
	else if (Attribute == GetHealthAttribute()) { NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth()); }
	else if (Attribute == GetIncomingDamageAttribute()) { NewValue = FMath::Max(NewValue, 0.0f); }
}

void UNF_DeployableHealthAttribute::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void UNF_DeployableHealthAttribute::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void UNF_DeployableHealthAttribute::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
	const UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (ASC && ASC->IsOwnerActorAuthoritative() && Attribute == GetMaxHealthAttribute() && GetHealth() > NewValue)
	{
		SetHealth(FMath::Max(NewValue, 0.0f));
	}
}

void UNF_DeployableHealthAttribute::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float Damage = GetIncomingDamage();
		SetIncomingDamage(0.0f);
		if (FMath::IsFinite(Damage) && Damage > 0.0f) { SetHealth(FMath::Clamp(GetHealth() - Damage, 0.0f, GetMaxHealth())); }
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute() || Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
}

void UNF_DeployableHealthAttribute::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNF_DeployableHealthAttribute, Health, OldValue);
}

void UNF_DeployableHealthAttribute::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UNF_DeployableHealthAttribute, MaxHealth, OldValue);
}
