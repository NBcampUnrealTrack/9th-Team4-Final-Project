#include "NFGA_MultiPurposeBase.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"


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
	if (IsValid(Character) == false)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}
	
	auto* Movement = Character->GetCharacterMovement();
	if (IsValid(Movement) == false)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}
	
	Movement->StopMovementImmediately();
	Movement->DisableMovement();
}
