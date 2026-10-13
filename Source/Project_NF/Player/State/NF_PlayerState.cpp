
#include "NF_PlayerState.h"

#include "AbilitySystemComponent.h"
#include "GAS/Foundation/NF_AbilitySystemComponent.h"
#include "GAS/Tags/NFTag_TagGroup.h"
#include "Player/Character/NF_PlayerStatusAttribute.h"

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

void ANF_PlayerState::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority() == true)
	{
		NF_PlayerID = GetPlayerId();
	}
	
}
