
#include "NF_PlayerState.h"

#include "AbilitySystemComponent.h"
#include "GAS/Attributes/NF_KeyHolderAttributeSet.h"
#include "GAS/Foundation/NF_AbilitySystemComponent.h"
#include "Player/Data/NF_PlayerDataAsset.h" 
#include "Player/Character/NF_PlayerStatusAttribute.h"
#include "Net/UnrealNetwork.h"
#include "Player/Character/NFGA_MeleeAttack.h"

ANF_PlayerState::ANF_PlayerState()
{
	//Net
	SetNetUpdateFrequency(100.0f);
	
	//ASC
	PlayerASC = CreateDefaultSubobject<UNF_AbilitySystemComponent>(TEXT("PlayerASC"));
	PlayerASC->SetIsReplicated(true);
	PlayerASC->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	PlayerStatusAttribute = CreateDefaultSubobject<UNF_PlayerStatusAttribute>(TEXT("PlayerStatusAttribute"));
	KeyHolderAttributeSet = CreateDefaultSubobject<UNF_KeyHolderAttributeSet>(TEXT("KeyHolderAttributeSet"));
}

void ANF_PlayerState::InitAbilitySystem(APawn* AvatarPawn)
{
	if (IsValid(PlayerASC) == false) return;
	if (IsValid(AvatarPawn) == false) return;

	PlayerASC->InitAbilityActorInfo(this, AvatarPawn);

	GiveDefaultAbility();
}

void ANF_PlayerState::GiveDefaultAbility()
{
	if (HasAuthority() == false) return;
	if (bIsGiveAbility == true) return;

	for (const auto& EachAbility : NF_PlayerDataAsset->GameplayAbilityArray)
	{
		if (IsValid(EachAbility) == false) continue;

		if (PlayerASC->FindAbilitySpecFromClass(EachAbility) == nullptr)
		{
			PlayerASC->GiveAbility(FGameplayAbilitySpec(EachAbility, 1, INDEX_NONE, NF_PlayerDataAsset));
		}
	}

	bIsGiveAbility = true;
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
	ConfigDataAsset();
}


void ANF_PlayerState::ConfigDataAsset()
{
	checkf(IsValid(NF_PlayerDataAsset) == true, TEXT("[NF_PlayerState]의 [NF_PlayerDataAsset]가  삽입되어있지않습니다."));
	checkf(IsValid(NF_PlayerDataAsset->MeleeAttack_Health_GEClass) == true, TEXT("[NF_PlayerDataAsset]의 [MeleeAttack_GEClass]가  삽입되어있지않습니다."));
	checkf(IsValid(NF_PlayerDataAsset->MeleeAttack_Shield_GEClass) == true, TEXT("[NF_PlayerDataAsset]의 [MeleeAttack_GEClass]가  삽입되어있지않습니다."));
	
}