#include "GamePlay/Combat/Boss/NF_BossAttributeSet.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "GamePlay/Combat/Boss/NF_BossBase.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"
#include "Player/State/NF_PlayerState.h"

UNF_BossAttributeSet::UNF_BossAttributeSet()
{
	InitMaxHealth(10000.0f);
	InitHealth(GetMaxHealth());
	InitIncomingDamage(0.0f);
}

void UNF_BossAttributeSet::InitializeForBoss(const float InMaxHealth)
{
	const float SafeMaxHealth = FMath::Max(InMaxHealth, 1.0f);
	SetMaxHealth(SafeMaxHealth);
	SetHealth(SafeMaxHealth);
}

void UNF_BossAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
	else if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetIncomingDamageAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

bool UNF_BossAttributeSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	const bool bCanExecute = Super::PreGameplayEffectExecute(Data);
	if (!bCanExecute)
	{
		return false;
	}

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		HealthBeforeGameplayEffect = GetHealth();
		bHasHealthBeforeGameplayEffect = true;
	}

	return true;
}

void UNF_BossAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// Standard damage path (same as monsters/players): positive IncomingDamage is subtracted from Health.
	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float Damage = GetIncomingDamage();
		SetIncomingDamage(0.0f);
		if (!FMath::IsFinite(Damage) || Damage <= 0.0f)
		{
			return;
		}
		const float HealthBefore = GetHealth();
		const float HealthAfter = FMath::Clamp(HealthBefore - Damage, 0.0f, GetMaxHealth());
		SetHealth(HealthAfter);
		NotifyBossHealthChanged(Data, HealthBefore, HealthAfter);
		return;
	}

	// Legacy path kept for effects that still modify Health directly with a negative magnitude (e.g. Thorns).
	if (Data.EvaluatedData.Attribute != GetHealthAttribute())
	{
		return;
	}

	const float HealthAfterGameplayEffect = FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth());
	const float HealthBeforeGameplayEffectValue = bHasHealthBeforeGameplayEffect
		? HealthBeforeGameplayEffect : HealthAfterGameplayEffect;
	bHasHealthBeforeGameplayEffect = false;
	SetHealth(HealthAfterGameplayEffect);
	NotifyBossHealthChanged(Data, HealthBeforeGameplayEffectValue, HealthAfterGameplayEffect);
}

void UNF_BossAttributeSet::NotifyBossHealthChanged(const FGameplayEffectModCallbackData& Data,
	const float HealthBeforeGameplayEffectValue, const float HealthAfterGameplayEffect)
{
	ANF_BossBase* Boss = Cast<ANF_BossBase>(GetOwningActor());
	if (!Boss || !Boss->HasAuthority())
	{
		return;
	}

	const float EffectiveDamage = FMath::Max(HealthBeforeGameplayEffectValue - HealthAfterGameplayEffect, 0.0f);
	if (EffectiveDamage > 0.0f)
	{
		UAbilitySystemComponent* InstigatorASC =
			Data.EffectSpec.GetContext().GetOriginalInstigatorAbilitySystemComponent();
		AActor* InstigatorAvatar = InstigatorASC ? InstigatorASC->GetAvatarActor() : nullptr;
		APawn* InstigatorPawn = Cast<APawn>(InstigatorAvatar);
		ANF_PlayerState* InstigatorPlayerState = InstigatorPawn
			? InstigatorPawn->GetPlayerState<ANF_PlayerState>() : nullptr;
		if (!InstigatorPlayerState && InstigatorASC)
		{
			InstigatorPlayerState = Cast<ANF_PlayerState>(InstigatorASC->GetOwnerActor());
		}

		Boss->RecordDamage(InstigatorPlayerState, EffectiveDamage, HealthAfterGameplayEffect <= 0.0f);
	}

	Boss->HandleHealthUpdated(GetHealth());
}

void UNF_BossAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MaxHealth, COND_None, REPNOTIFY_Always);
}

void UNF_BossAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Health, OldValue);
}

void UNF_BossAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MaxHealth, OldValue);
}
