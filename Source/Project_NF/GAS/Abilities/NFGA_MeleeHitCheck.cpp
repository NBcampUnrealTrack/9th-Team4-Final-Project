#include "NFGA_MeleeHitCheck.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "GAS/Foundation/NF_TargetActorTrace.h"
#include "GAS/Tags/NFTag_Player.h"
#include "Player/Data/NF_PlayerDataAsset.h"

UNFGA_MeleeHitCheck::UNFGA_MeleeHitCheck()
{
	// BPToCPP Auto-Generated Tag Setup Begin
	{
		FAbilityTriggerData TriggerData;
		TriggerData.TriggerTag = FGameplayTag(NFTag_Player::Player_Event_HitCheck);
		TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		AbilityTriggers.Add(TriggerData);
	}
	// BPToCPP Auto-Generated Tag Setup End

	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}


void UNFGA_MeleeHitCheck::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	auto* NF_PlayerDataAsset = Cast<UNF_PlayerDataAsset>(GetCurrentSourceObject());
	if (IsValid(NF_PlayerDataAsset) == false) return EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	
	HitCheck(NF_PlayerDataAsset->MeleeAttack_Health_GEClass, NF_PlayerDataAsset->MeleeAttackDamage);
	
	UAbilityTask_WaitGameplayEvent* WaitGameplayEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, NFTag_Player::Player_Event_HitCheckEnd, nullptr, false, true);
	if (IsValid(WaitGameplayEvent) == false) return EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	
	WaitGameplayEvent->EventReceived.AddDynamic(this, &ThisClass::OnEndHitCheck);
	WaitGameplayEvent->ReadyForActivation(); //필수적으로 이걸 사용해야준비가 된다.
	
}


void UNFGA_MeleeHitCheck::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


void UNFGA_MeleeHitCheck::OnEndHitCheck(FGameplayEventData EventData)
{
	if (IsLocallyControlled() == true)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}


#pragma region HitCheck
void UNFGA_MeleeHitCheck::HitCheck(TSubclassOf<UGameplayEffect> GameplayEffectClass, float PayloadMagnitude)
{
	PayloadMagnitude_CallBack = PayloadMagnitude;
	GameplayEffectClass_CallBack = GameplayEffectClass;
	
	UAbilityTask_WaitTargetData* HitCheckData = UAbilityTask_WaitTargetData::WaitTargetData(
		this, 
		TEXT("HitCheckTask"),
		EGameplayTargetingConfirmation::CustomMulti,
		ANF_TargetActorTrace::StaticClass());
	
	if (IsValid(HitCheckData) == false) return EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	
	HitCheckData->ValidData.AddDynamic(this, &ThisClass::OnHitCheckValidData);
	
	//내 Trace 클래스를 기준으로 타겟 검사 실행
	AGameplayAbilityTargetActor* SpawnedActor = nullptr;
	if (HitCheckData->BeginSpawningActor(this, ANF_TargetActorTrace::StaticClass(), SpawnedActor) == true)
	{
		SpawnedActor->bDebug = true;
		HitCheckData->FinishSpawningActor(this, SpawnedActor);
	}
	
	HitCheckData->ReadyForActivation();
}


void UNFGA_MeleeHitCheck::OnHitCheckValidData(const FGameplayAbilityTargetDataHandle& Data)
{
	if (HasAuthority(&CurrentActivationInfo) == true)
	{
		if (UAbilitySystemBlueprintLibrary::TargetDataHasActor(Data, 0) == false) return;

		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(GameplayEffectClass_CallBack, 1.0f);

		UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle,
			FGameplayTag::RequestGameplayTag(TEXT("Data.Damage")), PayloadMagnitude_CallBack);

		K2_ApplyGameplayEffectSpecToTarget(SpecHandle, Data);
	}
}
#pragma endregion 

