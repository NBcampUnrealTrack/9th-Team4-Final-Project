#include "NFGA_MultiPurposeBase.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"
#include "GamePlay/Combat/Weapons/NF_WeaponComponent.h"
#include "Player/Character/NF_Player.h"
#include "Player/Data/NF_PlayerDataAsset.h"


void UNFGA_MultiPurposeBase::OnTaskEnd()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}


void UNFGA_MultiPurposeBase::OnWaitTaskEnd(FGameplayEventData EventData)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}


void UNFGA_MultiPurposeBase::OnPlayerDisableMovement()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (IsValid(Character) == false) return;
	
	auto* Movement = Character->GetCharacterMovement();
	if (IsValid(Movement) == false) return;
	
	Movement->StopMovementImmediately();
	Movement->DisableMovement();
}

void UNFGA_MultiPurposeBase::AttachWeaponToSocket(UStaticMeshComponent* WeaponMeshComponent, FName WeaponSocketNameToUsed)
{
	auto* NF_PlayerDataAsset = Cast<UNF_PlayerDataAsset>(GetCurrentSourceObject());
	if (IsValid(NF_PlayerDataAsset) == false) return;
	
	auto* NF_Player = Cast<ANF_Player>(GetAvatarActorFromActorInfo());
	if (IsValid(NF_Player) == false) return;
	
	WeaponMeshComponent = NF_Player->MeleeAttackMeshComponent;
	if (IsValid(WeaponMeshComponent) == false) return;
	
	const auto& VisualMesh = NF_Player->VisualMesh;
	if (IsValid(VisualMesh) == false) return;
	
	WeaponMeshComponent->AttachToComponent(VisualMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocketNameToUsed);
}
