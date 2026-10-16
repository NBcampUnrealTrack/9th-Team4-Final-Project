
#include "NFGA_Death.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/Tags/NFTag_Player.h"
#include "Kismet/KismetSystemLibrary.h"


UNFGA_Death::UNFGA_Death()
{
	AbilityTags.AddTag(NFTag_Player::Player_Ability_Death);
}


void UNFGA_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                  const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	UKismetSystemLibrary::PrintString(GetAvatarActorFromActorInfo(),
									  (TEXT("[Death] GA가 실행됐습니다.]")),
									  true, true, FColor::Magenta, 10.0f);
	
	auto* AvatarCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (IsValid(AvatarCharacter) == false) return;
	
	AvatarCharacter->GetCharacterMovement()->SetMovementMode(MOVE_None);
	
	//EndAbility(Handle, ActorInfo, ActivationInfo, true , false);
}


void UNFGA_Death::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                          const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

