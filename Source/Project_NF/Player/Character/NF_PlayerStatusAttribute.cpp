// Fill out your copyright notice in the Description page of Project Settings.


#include "NF_PlayerStatusAttribute.h"

#include "GameplayEffectExtension.h"
#include "Data/HelperFunction/NF_GASFunctionLibrary.h"
#include "GameFramework/PlayerState.h"
#include "GAS/Tags/NFTag_Player.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"


UNF_PlayerStatusAttribute::UNF_PlayerStatusAttribute()
{
	InitHealth(GetMaxHealth());
	//InitDefence(GetMaxDefence());
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
	
	UKismetSystemLibrary::PrintString(GetOwningActor(),
		FString::Printf(TEXT("데미지 = %.2f 남은 체력 = %.2f 남은 쉴드 = %.2f"),
		Data.EvaluatedData.Magnitude, GetHealth(), GetShield()),
		true, true, FLinearColor::Black, 5.0f);
	
	IsOutOfHealth(Data);
	IsOutOfShield(Data);
}


void UNF_PlayerStatusAttribute::IsOutOfHealth(const FGameplayEffectModCallbackData& Data)
{
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp<float>(GetHealth(), 0.0f, GetMaxHealth()));
		
		if (GetHealth() <= 0.0f && bIsOutOfHealth == false)
		{
			bIsOutOfHealth = true;
			
			LethalDamageInstigatorPlayerState = Cast<APlayerState>(Data.EffectSpec.GetContext().GetOriginalInstigator());
			OnOutOfHealth.Broadcast();
		}
		else if (GetHealth() > 0.0f)
		{
			bIsOutOfHealth = false;
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
			bIsOutOfHealth = false;
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
	//DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MaxDefence, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Defence, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MaxShield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Shield, COND_None, REPNOTIFY_Always);
}


void UNF_PlayerStatusAttribute::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MaxHealth, OldMaxHealth);
}


void UNF_PlayerStatusAttribute::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Health, OldHealth);
}


/*void UNF_PlayerStatusAttribute::OnRep_MaxDefence(const FGameplayAttributeData& OldMaxDefence)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MaxDefence, OldMaxDefence);
}*/


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



