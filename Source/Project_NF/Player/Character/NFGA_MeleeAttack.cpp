

#include "NFGA_MeleeAttack.h"

#include "NF_Player.h"
#include "Data/HelperFunction/NF_GASFunctionLibrary.h"
#include "GameFramework/Character.h"
#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"
#include "GamePlay/Combat/Weapons/NF_WeaponComponent.h"
#include "GAS/Tags/NFTag_Player.h"
#include "Player/Data/NF_PlayerDataAsset.h"
#include "UpperBodyBlend/UpperBodyLayer.h"

UNFGA_MeleeAttack::UNFGA_MeleeAttack()
{
	// BPToCPP Auto-Generated Tag Setup Begin
	CancelAbilitiesWithTag.AddTag(FGameplayTag(NFTag_Player::Player_State_Death));
	CancelAbilitiesWithTag.AddTag(FGameplayTag(NFTag_Player::Player_State_Traversal));

	ActivationOwnedTags.AddTag(FGameplayTag(NFTag_Player::Player_State_Attack));

	BlockAbilitiesWithTag.AddTag(FGameplayTag(NFTag_Player::Player_Ability_Fire));
	BlockAbilitiesWithTag.AddTag(FGameplayTag(NFTag_Player::Player_Ability_Reload));
	BlockAbilitiesWithTag.AddTag(FGameplayTag(NFTag_Player::Player_Ability_Zoom));
	
	ActivationBlockedTags.AddTag(FGameplayTag(NFTag_Player::Player_State_Reload));
	ActivationBlockedTags.AddTag(FGameplayTag(NFTag_Player::Player_State_Death));
	ActivationBlockedTags.AddTag(FGameplayTag(NFTag_Player::Player_State_Attack));
	ActivationBlockedTags.AddTag(FGameplayTag(NFTag_Player::Player_State_Traversal));
	// BPToCPP Auto-Generated Tag Setup End
}


void UNFGA_MeleeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo, 
                                        const FGameplayAbilityActivationInfo ActivationInfo,const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	auto* NF_PlayerDataAsset = Cast<UNF_PlayerDataAsset>(GetCurrentSourceObject());
	if (IsValid(NF_PlayerDataAsset) == false) return EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	
	auto* NF_Player = Cast<ANF_Player>(GetAvatarActorFromActorInfo());
	if (IsValid(NF_Player) == false) return EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	
	
	const auto& NF_WeaponComponent = NF_Player->NF_WeaponComponent;
	if (IsValid(NF_WeaponComponent) == true && IsValid(NF_WeaponComponent->GetEquippedWeapon()) == true)
	{
		EquippedWeaponType = NF_WeaponComponent->GetEquippedWeaponType();
		EquippedWeaponSocketName = NF_WeaponComponent->GetEquippedWeaponSocketName();
		AttachWeaponToSocket(NF_WeaponComponent->GetEquippedWeaponMesh(), NF_PlayerDataAsset->WeaponKeepSocket);
	}
	else return EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	
	NF_Player->SetNF_WeaponType(ENF_WeaponType::Dagger);
	
	AttachWeaponToSocket(NF_Player->MeleeAttackMeshComponent, NF_PlayerDataAsset->MeleeAttackSocket);
	
	if (NF_Player->IsLocallyControlled() == true)
	{
		UUpperBodyLayerLibrary::SetUpperBodyCombatStanceByFire(NF_Player, true);
	}
	
	//AttackMontage에서 노티파이로 히트체크GA 호출함
	UNF_GASFunctionLibrary::NF_PlayMontageAndWaitWithEndAbility(this, NF_PlayerDataAsset->MeleeAttackMontage);
}


void UNFGA_MeleeAttack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	auto* NF_PlayerDataAsset = Cast<UNF_PlayerDataAsset>(GetCurrentSourceObject());
	if (IsValid(NF_PlayerDataAsset) == false) return EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	
	auto* NF_Player = Cast<ANF_Player>(GetAvatarActorFromActorInfo());
	if (IsValid(NF_Player) == false) return Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled); 
	if (IsValid(NF_Player->MeleeAttackMeshComponent) == false) return Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	
	if (NF_Player->IsLocallyControlled() == true)
	{
		UUpperBodyLayerLibrary::SetUpperBodyCombatStanceByFire(NF_Player, false);
	}
	
	const auto& NF_WeaponComponent = NF_Player->NF_WeaponComponent;
	if (IsValid(NF_WeaponComponent) == true && IsValid(NF_WeaponComponent->GetEquippedWeapon()) == true)
	{
		AttachWeaponToSocket(NF_WeaponComponent->GetEquippedWeaponMesh(), EquippedWeaponSocketName);
		AttachWeaponToSocket(NF_Player->MeleeAttackMeshComponent);
		NF_Player->SetNF_WeaponType(EquippedWeaponType);
		return Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	}
	
	AttachWeaponToSocket(NF_Player->MeleeAttackMeshComponent);
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
