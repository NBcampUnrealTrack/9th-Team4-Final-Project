// Fill out your copyright notice in the Description page of Project Settings.


#include "NF_PlayerStatusAttribute.h"

#include "GameplayEffectExtension.h"
#include "Data/HelperFunction/NF_GASFunctionLibrary.h"
#include "GameFramework/PlayerState.h"
#include "GAS/Data/NF_PlayerGEDataAsset.h"
#include "GAS/Manager/NF_GEManagerSubsystem.h"
#include "GAS/Tags/NFTag_Player.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Player/Data/NF_PlayerDataAsset.h"
#include "Player/State/NF_PlayerState.h"


UNF_PlayerStatusAttribute::UNF_PlayerStatusAttribute()
{
	InitHealth(GetMaxHealth());
	InitShield(GetMaxShield());
}


void UNF_PlayerStatusAttribute::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	
	
	
	if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max<float>(1.0f, NewValue);
	}
	
	else if (Attribute == GetHealthAttribute())
	{
		if (GetShield() > 0.0f)
		{
			NewValue = GetHealth();
			return;
		}
		
		float ReceivedDamage = GetHealth() - NewValue;
		
		if (ReceivedDamage > 0.0f)
		{
			UNF_GASFunctionLibrary::NF_SendGameplayEventTagToActorWithPayloadData(
			GetOwningAbilitySystemComponent()->GetAvatarActor(),
			NFTag_Player::Player_Event_DamagedCheck,
			ReceivedDamage);
		}
		
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, GetMaxHealth());
	}
	

	if (Attribute == GetMaxShieldAttribute())
	{
		NewValue = FMath::Max<float>(1.0f, NewValue);
	}
	
	else if (Attribute == GetShieldAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, GetMaxShield());
	}
}


void UNF_PlayerStatusAttribute::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	
	
	if (GetIsDamagedByTeam(Data) == true) return;
	if (Data.EvaluatedData.Attribute == GetInComingDamageAttribute())
	{
		UNF_GASFunctionLibrary::NF_SendGameplayEventTagToActorWithPayloadData(this,NFTag_Player::Player_Event_HitReact);
	}
	
	SetShield(GetShield()-GetInComingDamage());
	SetHealth(GetHealth()-(GetInComingDamage()-GetDefence()));
	SetInComingDamage(0.0f);
	
	IsOutOfHealth(Data);
	IsOutOfShield(Data);
	
	BlockShieldRecovery(Data);
}


bool UNF_PlayerStatusAttribute::GetIsDamagedByTeam(const FGameplayEffectModCallbackData& Data)
{
	auto* AttackerPlayerState = Cast<ANF_PlayerState>(Data.EffectSpec.GetContext().GetOriginalInstigator());
	auto* VictimPlayerState = Cast<ANF_PlayerState>(GetOwningActor());
	if (AttackerPlayerState == nullptr || VictimPlayerState == nullptr) return false;
	
	if (AttackerPlayerState->GetNFTeamID() == VictimPlayerState->GetNFTeamID())
	{
		SetInComingDamage(0.0f);
		return  true;
	}
	
	return false;
}

void UNF_PlayerStatusAttribute::BlockShieldRecovery(const FGameplayEffectModCallbackData& Data)
{
	bool bIsDamaged = false;
	if (Data.EvaluatedData.Attribute == GetInComingDamageAttribute() && Data.EvaluatedData.Magnitude > 0.0f)
	{
		bIsDamaged = true;
	}
	if (Data.EvaluatedData.Attribute == GetShieldAttribute() && Data.EvaluatedData.Magnitude < 0.0f)
	{
		bIsDamaged = true;
	}
	if (Data.EvaluatedData.Attribute == GetHealthAttribute() && Data.EvaluatedData.Magnitude < 0.0f)
	{
		bIsDamaged = true;
	}
	if (bIsDamaged == false) return;
	
	auto* PlayerState = Cast<ANF_PlayerState>(GetOwningActor());
	if (IsValid(PlayerState) == false) return;
	if (IsValid(PlayerState->NF_PlayerDataAsset) == false) return;
	
	UAbilitySystemComponent* ASC = &Data.Target;
	
	float ShieldRecoveryBlockedDuration = PlayerState->NF_PlayerDataAsset->ShieldRecoveryBlockedDuration;
	
	const auto& NF_ShieldRecoveryBlocked_GEClass = UNF_GEManagerSubsystem::FindEffect(this, &UNF_PlayerGEDataAsset::NF_ShieldRecoveryBlocked_GEClass);
	if (IsValid(NF_ShieldRecoveryBlocked_GEClass) == false) return;
	
	UNF_GASFunctionLibrary::NF_ApplyGameplayEffectToSelfWithSetByCaller(ASC, 
		NF_ShieldRecoveryBlocked_GEClass,ShieldRecoveryBlockedDuration,NFTag_Player::Player_Data_ShieldRecoveryBlockedDuration);
}

void UNF_PlayerStatusAttribute::IsOutOfHealth(const FGameplayEffectModCallbackData& Data)
{
	if (Data.EvaluatedData.Attribute == GetHealthAttribute() || Data.EvaluatedData.Attribute == GetInComingDamageAttribute())
	{
		SetHealth(FMath::Clamp<float>(GetHealth(), 0.0f, GetMaxHealth()));
		
		if (GetHealth() <= 0.0f && bIsOutOfHealth == false)
		{
			bIsOutOfHealth = true;
			
			GetOwningAbilitySystemComponent()->AddLooseGameplayTag(NFTag_Player::Player_State_Death);
			LethalDamageInstigatorPlayerState = Cast<APlayerState>(Data.EffectSpec.GetContext().GetOriginalInstigator());
			OnOutOfHealth.Broadcast();
		}
		
		if (GetHealth() > 0.0f && bIsOutOfHealth == true)
		{
			bIsOutOfHealth = false;
			GetOwningAbilitySystemComponent()->RemoveLooseGameplayTag(NFTag_Player::Player_State_Death);
			//TODO 체력이 바뀜에 대한 브로드캐스트?
		}
	}
}


void UNF_PlayerStatusAttribute::IsOutOfShield(const FGameplayEffectModCallbackData& Data)
{
	if (Data.EvaluatedData.Attribute == GetShieldAttribute())
	{
		SetShield(FMath::Clamp<float>(GetShield(), 0.0f, GetMaxShield()));
		
		if (GetShield() <= 0.0f && bIsOutOfShield == false)
		{
			bIsOutOfShield = true;
			OnOutOfShield.Broadcast();
		}
		else if (GetShield() > 0.0f)
		{
			bIsOutOfShield = false;
			//TODO 쉴드량이 바뀜에 대한 브로드캐스트?
		}
	}
}


APlayerState* UNF_PlayerStatusAttribute::GetLethalDamageInstigatorPlayerState() const
{
	return LethalDamageInstigatorPlayerState.Get();
}


void UNF_PlayerStatusAttribute::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, InComingDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Defence, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MaxShield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Shield, COND_None, REPNOTIFY_Always);
	
	DOREPLIFETIME(ThisClass, bIsOutOfHealth);
}


void UNF_PlayerStatusAttribute::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MaxHealth, OldMaxHealth);
}


void UNF_PlayerStatusAttribute::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Health, OldHealth);
}

void UNF_PlayerStatusAttribute::OnRep_InComingDamage(const FGameplayAttributeData& OldInComingDamage)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, InComingDamage, OldInComingDamage);
}


void UNF_PlayerStatusAttribute::OnRep_Defence(const FGameplayAttributeData& OldDefence)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Defence, OldDefence);
}


void UNF_PlayerStatusAttribute::OnRep_MaxShield(const FGameplayAttributeData& OldMaxShield)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MaxShield, OldMaxShield);
}


void UNF_PlayerStatusAttribute::OnRep_Shield(const FGameplayAttributeData& OldShield)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Shield, OldShield);
}



