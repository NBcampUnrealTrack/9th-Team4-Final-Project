#include "GamePlay/Combat/Boss/NF_BossAttributeSet.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "GamePlay/Combat/Boss/NF_BossBase.h"
#include "Net/UnrealNetwork.h"
#include "Player/Character/NF_Player.h"

UNF_BossAttributeSet::UNF_BossAttributeSet()
{
	InitMaxHealth(10000.0f);
	InitHealth(GetMaxHealth());
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
}

void UNF_BossAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	if (Data.EvaluatedData.Attribute != GetHealthAttribute())
	{
		return;
	}

	SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	ANF_BossBase* Boss = Cast<ANF_BossBase>(GetOwningActor());
	if (!Boss || !Boss->HasAuthority())
	{
		return;
	}

	const float AppliedMagnitude = Data.EvaluatedData.Magnitude;
	if (AppliedMagnitude < 0.0f)
	{
		UAbilitySystemComponent* InstigatorASC = Data.EffectSpec.GetContext().GetOriginalInstigatorAbilitySystemComponent();
		AActor* InstigatorAvatar = InstigatorASC ? InstigatorASC->GetAvatarActor() : nullptr;
		ANF_Player* Player = Cast<ANF_Player>(InstigatorAvatar);
		if (!Player && InstigatorASC)
		{
			Player = Cast<ANF_Player>(InstigatorASC->GetOwnerActor());
		}
		Boss->RecordDamage(Player, -AppliedMagnitude);
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
