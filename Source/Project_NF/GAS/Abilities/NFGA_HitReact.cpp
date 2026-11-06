#include "NFGA_HitReact.h"

#include "Data/HelperFunction/NF_GASFunctionLibrary.h"
#include "DSP/BufferDiagnostics.h"
#include "GAS/Tags/NFTag_Player.h"
#include "Player/Data/NF_PlayerDataAsset.h"

UNFGA_HitReact::UNFGA_HitReact()
{
	// BPToCPP Auto-Generated Tag Setup Begin
	{
		FAbilityTriggerData TriggerData;
		TriggerData.TriggerTag = FGameplayTag(NFTag_Player::Player_Event_HitReact);
		TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		AbilityTriggers.Add(TriggerData);
	}
	// BPToCPP Auto-Generated Tag Setup End
	
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	bRetriggerInstancedAbility = true;
}

void UNFGA_HitReact::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	UE_LOG(LogTemp, Warning,TEXT("Action HitReact!!"));
	
	auto* NF_PlayerDataAsset = Cast<UNF_PlayerDataAsset>(GetCurrentSourceObject());
	if (IsValid(NF_PlayerDataAsset) == false) return EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	
	if (bIsBeenAlreadyAction == false)
	{
		bIsBeenAlreadyAction = true;
		HitReactMontage = NF_PlayerDataAsset->HitReact_01;
	}

	else
	{
		bIsBeenAlreadyAction = false;
		HitReactMontage = NF_PlayerDataAsset->HitReact_02;
	}
	
	UNF_GASFunctionLibrary::NF_PlayMontageAndWaitWithEndAbility(this, HitReactMontage);
}

