
#include "NFGA_Death.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Data/HelperFunction/NF_GASFunctionLibrary.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/Tags/NFTag_Player.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Player/Data/NF_PlayerDataAsset.h"


UNFGA_Death::UNFGA_Death()
{
	AbilityTags.AddTag(NFTag_Player::Player_Ability_Death);
	
	ActivationOwnedTags.AddTag(NFTag_Player::Player_State_Death);
}


void UNFGA_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                  const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	UKismetSystemLibrary::PrintString(GetAvatarActorFromActorInfo(),
									  (TEXT("[Death] GA가 실행됐습니다.]")),
									  true, true, FColor::Magenta, 10.0f);
	
	auto* AvatarCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (IsValid(AvatarCharacter) == false) return EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	
	AvatarCharacter->GetCharacterMovement()->SetMovementMode(MOVE_None);
	
	auto* NF_PlayerDataAsset = Cast<UNF_PlayerDataAsset>(GetCurrentSourceObject());
	if (IsValid(NF_PlayerDataAsset) == false) return EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	
	auto* MontageTask = UNF_GASFunctionLibrary::NF_PlayMontageAndWait(this,NF_PlayerDataAsset->DeathMontage, false);
	if (MontageTask == nullptr) return EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	
	//MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnCompleteMontage);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnFailedMontage);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnFailedMontage);
	MontageTask->ReadyForActivation();
	
	
	UAbilityTask_WaitGameplayEvent* WaitGameplayEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, NFTag_Player::Player_Event_Destroy, nullptr, false, true);
	if (IsValid(WaitGameplayEvent) == false) return EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	
	WaitGameplayEvent->EventReceived.AddDynamic(this, &ThisClass::OnCompleteMontage);
	WaitGameplayEvent->ReadyForActivation();
}


void UNFGA_Death::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                          const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


void UNFGA_Death::OnFailedMontage()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}


void UNFGA_Death::OnCompleteMontage(FGameplayEventData EventData)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (IsValid(AvatarActor) == true)      
	{
		AvatarActor->SetLifeSpan(3.0f);
	}
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

