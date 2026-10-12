
#include "NF_PlayerState.h"

#include "AbilitySystemComponent.h"
#include "GAS/Foundation/NF_AbilitySystemComponent.h"

ANF_PlayerState::ANF_PlayerState()
{
	//Net
	SetNetUpdateFrequency(100.0f);
	
	//ASC
	NF_ASC = CreateDefaultSubobject<UNF_AbilitySystemComponent>(TEXT("NF_ASC"));
	NF_ASC->SetIsReplicated(true);
	NF_ASC->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	
	//Attribute Set
	
}

class UAbilitySystemComponent* ANF_PlayerState::GetAbilitySystemComponent() const
{
	return NF_ASC;
}

void ANF_PlayerState::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority() == true)
	{
		NF_PlayerID = GetPlayerId();
	}
}
