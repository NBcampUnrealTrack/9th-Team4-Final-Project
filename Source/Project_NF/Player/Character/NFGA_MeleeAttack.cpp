

#include "NFGA_MeleeAttack.h"

#include "Data/HelperFunction/NF_GASFunctionLibrary.h"
#include "GAS/Tags/NFTag_Player.h"
#include "Player/Data/NF_PlayerDataAsset.h"

UNFGA_MeleeAttack::UNFGA_MeleeAttack()
{
	// BPToCPP Auto-Generated Tag Setup Begin
	CancelAbilitiesWithTag.AddTag(FGameplayTag(NFTag_Player::Player_State_Death));
	CancelAbilitiesWithTag.AddTag(FGameplayTag(NFTag_Player::Player_State_Traversal));

	ActivationOwnedTags.AddTag(FGameplayTag(NFTag_Player::Player_State_Attack));

	ActivationBlockedTags.AddTag(FGameplayTag(NFTag_Player::Player_State_Death));
	ActivationBlockedTags.AddTag(FGameplayTag(NFTag_Player::Player_State_Attack));
	ActivationBlockedTags.AddTag(FGameplayTag(NFTag_Player::Player_State_Traversal));
	// BPToCPP Auto-Generated Tag Setup End
}


void UNFGA_MeleeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                        const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                        const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	
	auto* NF_PlayerDataAsset = Cast<UNF_PlayerDataAsset>(GetCurrentSourceObject());
	if (IsValid(NF_PlayerDataAsset) == false) return EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	
	UNF_GASFunctionLibrary::NF_PlayMontageAndWaitWithEndAbility(this, NF_PlayerDataAsset->MeleeAttackMontage);
	
}

void UNFGA_MeleeAttack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
