#include "NFGA_Downed.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GamePlay/Combat/Components/NF_PlayerLifeComponent.h"
#include "GAS/Tags/NFTag_LifeState.h"
#include "Kismet/KismetSystemLibrary.h"

UNFGA_Downed::UNFGA_Downed()
{
	// 캐릭마다 별도로 GA 상태 관리
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	
	// 서버에서 기절 행동 처리
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	
	AbilityTags.AddTag(NFTag_LifeState::Ability_Life_Downed);
}

void UNFGA_Downed::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	
	const bool bIsServer = ActorInfo && ActorInfo->IsNetAuthority();
	
	UKismetSystemLibrary::PrintString(Character,FString::Printf(TEXT("[Downed] GA 실행 - %s"),
		bIsServer ? TEXT("SERVER") : TEXT("CLIENT")), true, true,
		bIsServer ? FColor::Green : FColor::Magenta, 5.0f);
	
	if (!IsValid(Character))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	
	if (!IsValid(Movement))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	// 기절하면 현재 움직임을 멈춤
	Movement->StopMovementImmediately();
	// 기절 상태에서는 이동x (임시)
	Movement->DisableMovement();
}

void UNFGA_Downed::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	
	const bool bIsServer = ActorInfo && ActorInfo->IsNetAuthority();
	
	UKismetSystemLibrary::PrintString(
	Character,
	FString::Printf(
		TEXT("[Downed] GA 종료 - %s"),
		bIsServer ? TEXT("SERVER") : TEXT("CLIENT")),
	true, true,
	bIsServer ? FColor::Green : FColor::Magenta,
	5.0f);
	
	if (IsValid(Character))
	{
		UNF_PlayerLifeComponent* LifeComponent = Character->FindComponentByClass<UNF_PlayerLifeComponent>();
		
		// 소생해서 Alive가 되면 이동 가능
		if (IsValid(LifeComponent) && LifeComponent->GetLifeState() == ENFPlayerLifeState::Alive)
		{
			if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
			{
				Movement->SetMovementMode(MOVE_Walking);
			}
		}
	}
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
