
#include "NF_PlayerState.h"

#include "AbilitySystemComponent.h"
#include "GAS/Attributes/NF_KeyHolderAttributeSet.h"
#include "GAS/Foundation/NF_AbilitySystemComponent.h"
#include "GAS/Tags/NFTag_Player.h"
#include "Player/Data/NF_PlayerDataAsset.h" 
#include "Player/Character/NF_PlayerStatusAttribute.h"
#include "Net/UnrealNetwork.h"
#include "Player/Character/NFGA_MeleeAttack.h"
#include "Player/Character/NF_PlayerLevelAttribute.h"

ANF_PlayerState::ANF_PlayerState()
{
	//Net
	SetNetUpdateFrequency(100.0f);
	
	//ASC
	PlayerASC = CreateDefaultSubobject<UNF_AbilitySystemComponent>(TEXT("PlayerASC"));
	PlayerASC->SetIsReplicated(true);
	PlayerASC->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	
	PlayerStatusAttribute = CreateDefaultSubobject<UNF_PlayerStatusAttribute>(TEXT("PlayerStatusAttribute"));
	PlayerLevelAttribute = CreateDefaultSubobject<UNF_PlayerLevelAttribute>(TEXT("PlayerLevelAttribute"));
	KeyHolderAttributeSet = CreateDefaultSubobject<UNF_KeyHolderAttributeSet>(TEXT("KeyHolderAttributeSet"));
}

void ANF_PlayerState::InitAbilitySystem(APawn* AvatarPawn)
{
	if (IsValid(PlayerASC) == false) return;
	if (IsValid(AvatarPawn) == false) return;

	PlayerASC->InitAbilityActorInfo(this, AvatarPawn);

	GiveDefaultAbility();
	
	if (HasAuthority() == true && IsValid(PlayerStatusAttribute) == true)
	{
		const float MaxHealth = PlayerStatusAttribute->GetMaxHealth();
		const float MaxShield = PlayerStatusAttribute->GetMaxShield();
        
		PlayerASC->SetNumericAttributeBase(UNF_PlayerStatusAttribute::GetHealthAttribute(), MaxHealth);
		PlayerASC->SetNumericAttributeBase(UNF_PlayerStatusAttribute::GetShieldAttribute(), MaxShield);
	}
	
	PlayerShieldRecovery();
}

void ANF_PlayerState::GiveDefaultAbility()
{
	if (HasAuthority() == false) return;
	if (bIsGiveAbility == true) return;

	for (const auto& EachAbility : NF_PlayerDataAsset->NF_GameplayAbility_Array)
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
	if (HasAuthority() == false || NewTeamID < 1)
	{
		return false;
	}

	NF_TeamID = NewTeamID;
	OnTeamIDChanged.Broadcast();
	
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
	checkf(IsValid(NF_PlayerDataAsset->NF_SetDamage_GEClass) == true, TEXT("[NF_PlayerState]의 [NF_PlayerDataAsset]의 [NF_SetDamage_GEClass]가  삽입되어있지않습니다."));
	checkf(IsValid(NF_PlayerDataAsset->NF_ShieldRecovery_GEClass) == true, TEXT("[NF_PlayerState]의 [NF_PlayerDataAsset]의 [NF_ShieldRecovery_GEClass]가  삽입되어있지않습니다."));
}

void ANF_PlayerState::PlayerShieldRecovery()
{
	if (HasAuthority() == true)
	{
		if (bIsApplyShieldRecovery == true) return;
		
		if (IsValid(NF_PlayerDataAsset) == false) return;
		if (IsValid(NF_PlayerDataAsset->NF_ShieldRecovery_GEClass) == false) return;
		
		const FGameplayEffectSpecHandle EffectSpecHandle = 
			PlayerASC->MakeOutgoingSpec(NF_PlayerDataAsset->NF_ShieldRecovery_GEClass, 1.0f, PlayerASC->MakeEffectContext());
		if (EffectSpecHandle.IsValid() == false) return;
		
		EffectSpecHandle.Data->SetSetByCallerMagnitude(NFTag_Player::Player_Data_ShieldRecoveryAmount, NF_PlayerDataAsset->ShieldRecoveryAmount);
		PlayerASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
		
		bIsApplyShieldRecovery = true;
	}
}

void ANF_PlayerState::OnRep_NFTeamID()
{
	OnTeamIDChanged.Broadcast();
}
