#include "NFGA_Downed.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Data/HelperFunction/NF_GASFunctionLibrary.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GamePlay/Combat/Components/NF_PlayerLifeComponent.h"
#include "GAS/Tags/NFTag_LifeState.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Player/Data/NF_PlayerDataAsset.h"

UNFGA_Downed::UNFGA_Downed()
{
	// 캐릭마다 별도로 GA 상태 관리
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	
	// 서버에서 기절 행동 처리
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	
	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(NFTag_LifeState::Ability_Life_Downed);
	SetAssetTags(AssetTags);
}

void UNFGA_Downed::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	const bool bIsServer = ActorInfo && ActorInfo->IsNetAuthority();
	
	UKismetSystemLibrary::PrintString(this, FString::Printf(TEXT("[Downed] GA 실행 - %s"),
		bIsServer ? TEXT("SERVER") : TEXT("CLIENT")), true, true,
		bIsServer ? FColor::Green : FColor::Magenta, 5.0f);
	
	auto* NF_PlayerDataAsset = Cast<UNF_PlayerDataAsset>(GetCurrentSourceObject());
	if (IsValid(NF_PlayerDataAsset) == false) return EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	
	auto* MontageTask = UNF_GASFunctionLibrary::NF_PlayMontageAndWait(this, NF_PlayerDataAsset->DownedMontage);
	if (MontageTask == nullptr) return EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnTaskEnd);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnTaskEnd);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnTaskEnd);
	MontageTask->ReadyForActivation();
	
	OnPlayerDisableMovement();
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
