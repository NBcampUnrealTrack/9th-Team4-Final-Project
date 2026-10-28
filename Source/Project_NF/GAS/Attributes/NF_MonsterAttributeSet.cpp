#include "GAS/Attributes/NF_MonsterAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

UNF_MonsterAttributeSet::UNF_MonsterAttributeSet()
{
	// Default Attributes GE가 실패하거나 늦게 적용돼도
	// 몬스터가 0 HP로 생성되지 않게 하는 안전 초기값.
	InitMaxHealth(100.0f);
	InitHealth(100.0f);
	InitAttackPower(10.0f);
	InitMoveSpeed(300.0f);
	InitIncomingDamage(0.0f);
}

void UNF_MonsterAttributeSet::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(
		UNF_MonsterAttributeSet,
		Health,
		COND_None,
		REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(
		UNF_MonsterAttributeSet,
		MaxHealth,
		COND_None,
		REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(
		UNF_MonsterAttributeSet,
		AttackPower,
		COND_None,
		REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(
		UNF_MonsterAttributeSet,
		MoveSpeed,
		COND_None,
		REPNOTIFY_Always);
}

void UNF_MonsterAttributeSet::OnRep_Health(
	const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(
		UNF_MonsterAttributeSet,
		Health,
		OldHealth);
}

void UNF_MonsterAttributeSet::OnRep_MaxHealth(
	const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(
		UNF_MonsterAttributeSet,
		MaxHealth,
		OldMaxHealth);
}

void UNF_MonsterAttributeSet::OnRep_AttackPower(
	const FGameplayAttributeData& OldAttackPower)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(
		UNF_MonsterAttributeSet,
		AttackPower,
		OldAttackPower);
}

void UNF_MonsterAttributeSet::OnRep_MoveSpeed(
	const FGameplayAttributeData& OldMoveSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(
		UNF_MonsterAttributeSet,
		MoveSpeed,
		OldMoveSpeed);
}

void UNF_MonsterAttributeSet::PreAttributeChange(
	const FGameplayAttribute& Attribute,
	float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(
			NewValue,
			0.0f,
			GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
	else if (Attribute == GetAttackPowerAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetMoveSpeedAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UNF_MonsterAttributeSet::PostGameplayEffectExecute(
	const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute != GetIncomingDamageAttribute()) { return;	}

	const float Damage = GetIncomingDamage();

	UE_LOG(
	LogTemp,
	Warning,
	TEXT("Monster Damage Trace | GE=%s | Instigator=%s | Causer=%s | Damage=%.1f | Health=%.1f | MaxHealth=%.1f"),
	*GetNameSafe(Data.EffectSpec.Def),
	*GetNameSafe(Data.EffectSpec.GetContext().GetOriginalInstigator()),
	*GetNameSafe(Data.EffectSpec.GetContext().GetEffectCauser()),
	Damage,
	GetHealth(),
	GetMaxHealth());
	
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Monster damage received: %.1f / Health before: %.1f"),
		Damage,
		GetHealth());

	SetIncomingDamage(0.0f);

	if (Damage <= 0.0f)	{ return; }

	const float NewHealth = FMath::Clamp(
	GetHealth() - Damage,
	0.0f,
	GetMaxHealth());

	SetHealth(NewHealth);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Monster Health after damage: %.1f"),
		NewHealth);
	
	if (NewHealth <= 0.0f && bIsOutOfHealth == false)
	{
		bIsOutOfHealth = true;

		UE_LOG(LogTemp, Warning, TEXT("Monster out of health"));

		OnMonsterOutOfHealth.Broadcast();

		return;
	}

	AActor* DamageSource =
	Data.EffectSpec.GetContext().GetOriginalInstigator();

	if (!DamageSource)
	{
		DamageSource =
			Data.EffectSpec.GetContext().GetEffectCauser();
	}

	// Player ASCs are owned by PlayerState in this project. AI target selection
	// needs the possessed pawn, not the PlayerState actor from the effect context.
	if (const APlayerState* PlayerState = Cast<APlayerState>(DamageSource))
	{
		DamageSource = PlayerState->GetPawn();
	}

	OnMonsterDamaged.Broadcast(Damage, DamageSource);
}
