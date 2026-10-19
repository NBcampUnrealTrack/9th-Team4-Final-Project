
#include "NF_PlayerState.h"

#include "AbilitySystemComponent.h"
#include "GAS/Foundation/NF_AbilitySystemComponent.h"
#include "GAS/Tags/NFTag_TagGroup.h"
#include "Player/Character/NF_PlayerStatusAttribute.h"
#include "Net/UnrealNetwork.h"

ANF_PlayerState::ANF_PlayerState()
{
	//Net
	SetNetUpdateFrequency(100.0f);
	
	//ASC
	PlayerASC = CreateDefaultSubobject<UNF_AbilitySystemComponent>(TEXT("PlayerASC"));
	PlayerASC->SetIsReplicated(true);
	PlayerASC->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	
	PlayerStatusAttribute = CreateDefaultSubobject<UNF_PlayerStatusAttribute>(TEXT("PlayerStatusAttribute"));
	
}

class UAbilitySystemComponent* ANF_PlayerState::GetAbilitySystemComponent() const
{
	return PlayerASC;
}

bool ANF_PlayerState::SetNFTeamID(int32 NewTeamID)
{
	if (!HasAuthority() || NewTeamID < 1)
	{
		return false;
	}

	NF_TeamID = NewTeamID;
	ForceNetUpdate();
	return true;
}

void ANF_PlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, NF_TeamID);
}

void ANF_PlayerState::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority() == true)
	{
		NF_PlayerID = GetPlayerId();
	}
	
}
