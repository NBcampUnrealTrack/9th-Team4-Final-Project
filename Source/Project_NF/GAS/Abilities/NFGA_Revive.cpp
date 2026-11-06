#include "NFGA_Revive.h"
#include "GAS/Tags/NFTag_LifeState.h"
#include "GamePlay/Combat/Components/NF_PlayerLifeComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "TimerManager.h"

UNFGA_Revive::UNFGA_Revive()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	
	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(NFTag_LifeState::Ability_Life_Revive);
	SetAssetTags(AssetTags);
}

bool UNFGA_Revive::SetReviveTarget(APawn* InTargetPawn)
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	
	if (!ActorInfo || !ActorInfo->IsNetAuthority())
	{
		return false;
	}
	
	if (IsActive())
	{
		return false;
	}
	
	if (!IsValid(InTargetPawn) || InTargetPawn->IsActorBeingDestroyed())
	{
		return false;
	}
	
	if (ActorInfo->AvatarActor.Get() == InTargetPawn)
	{
		return false;
	}
	
	PendingReviveTarget = InTargetPawn;
	
	return true;
}

void UNFGA_Revive::ClearPendingReviveTarget()
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	
	if (!ActorInfo || !ActorInfo->IsNetAuthority() || IsActive())
	{
		return;
	}
	
	PendingReviveTarget.Reset();
}

void UNFGA_Revive::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	APawn* ReviverPawn = ActorInfo ? Cast<APawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	
	if (!ActorInfo || !ActorInfo->IsNetAuthority() || !IsValid(ReviverPawn))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	APawn* TargetPawn = PendingReviveTarget.Get();
	
	PendingReviveTarget.Reset();
	
	if (!IsValid(TargetPawn))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	UNF_PlayerLifeComponent* TargetLifeComponent =
		TargetPawn->FindComponentByClass<UNF_PlayerLifeComponent>();
	
	if (!IsValid(TargetLifeComponent) || !TargetLifeComponent->CanReviveFrom(ReviverPawn))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	ActiveReviveTarget = TargetPawn;
	
	UWorld* World = GetWorld();
	
	if (!IsValid(World))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	const float Duration = FMath::Max(ReviveDurationSeconds, 0.1f);
	
	World->GetTimerManager().SetTimer(
		ReviveTimerHandle, this, &UNFGA_Revive::CompleteRevive, Duration, false);
	
	World->GetTimerManager().SetTimer(
		ReviveValidationTimerHandle, this, &UNFGA_Revive::CheckReviveProgress, 0.1f, true);
}

void UNFGA_Revive::CompleteRevive()
{
	if (!IsActive())
	{
		return;
	}
	
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	
	APawn* ReviverPawn = ActorInfo ? Cast<APawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	
	APawn* TargetPawn = ActiveReviveTarget.Get();
	
	if (!ActorInfo || !ActorInfo->IsNetAuthority() || !IsValid(ReviverPawn) || !IsValid(TargetPawn))
	{
		EndAbility(GetCurrentAbilitySpecHandle(), ActorInfo, GetCurrentActivationInfo(), true, true);
		return;
	}
	
	UNF_PlayerLifeComponent* TargetLifeComponent =
		TargetPawn->FindComponentByClass<UNF_PlayerLifeComponent>();
	
	if (!IsValid(TargetLifeComponent) || !TargetLifeComponent->CanReviveFrom(ReviverPawn))
	{
		EndAbility(GetCurrentAbilitySpecHandle(), ActorInfo, GetCurrentActivationInfo(), true, true);
		return;
	}
	
	const bool bReviveSucceeded = TargetLifeComponent->Revive();
	
	EndAbility(GetCurrentAbilitySpecHandle(), ActorInfo, GetCurrentActivationInfo(), true, !bReviveSucceeded);
}

void UNFGA_Revive::CheckReviveProgress()
{
	if (!IsActive())
	{
		return;
	}
	
	const  FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	
	APawn* ReviverPawn = ActorInfo ? Cast<APawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	
	APawn* TargetPawn = ActiveReviveTarget.Get();
	
	UNF_PlayerLifeComponent* TargetLifeComponent =
		IsValid(TargetPawn) ? TargetPawn->FindComponentByClass<UNF_PlayerLifeComponent>() : nullptr;
	
	if (!ActorInfo || !ActorInfo->IsNetAuthority() || !IsValid(ReviverPawn) || !IsValid(TargetLifeComponent)
		|| !TargetLifeComponent->CanReviveFrom(ReviverPawn))
	{
		EndAbility(GetCurrentAbilitySpecHandle(), ActorInfo, GetCurrentActivationInfo(), true, true);
	}
}

void UNFGA_Revive::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReviveTimerHandle);
		World->GetTimerManager().ClearTimer(ReviveValidationTimerHandle);
	}
	
	PendingReviveTarget.Reset();
	ActiveReviveTarget.Reset();
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
