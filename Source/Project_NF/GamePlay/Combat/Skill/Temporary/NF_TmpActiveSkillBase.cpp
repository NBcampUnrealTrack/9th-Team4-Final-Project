#include "GamePlay/Combat/Skill/Temporary/NF_TmpActiveSkillBase.h"
#include "GamePlay/Combat/Skill/Temporary/NF_TmpSkillDataAsset.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

const UNF_TmpActiveSkillDataAsset* UNF_TmpActiveSkillBase::GetActiveData() const
{
	return Cast<UNF_TmpActiveSkillDataAsset>(GetSkillData());
}

bool UNF_TmpActiveSkillBase::BeginLocalInput()
{
	const auto* Info = GetCurrentActorInfo();
	const auto* Data = GetActiveData();
	auto* Avatar = GetAvatarActorFromActorInfo();
	if (!Info || !Info->IsLocallyControlled() || !Data || !IsValid(Avatar) || !GetWorld()
		|| IsActive() || !CanActivateAbility(CurrentSpecHandle, Info)) { return false; }
	CancelLocalInput(); bHoldingInput = true;
	GetWorld()->GetTimerManager().SetTimer(InputTimer, this, &ThisClass::CheckInputState, 0.05f, true);
	if (Data->PreviewActorClass)
	{
		FActorSpawnParameters Params; Params.Owner = Avatar; Params.Instigator = Cast<APawn>(Avatar);
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		PreviewActor = GetWorld()->SpawnActor<AActor>(Data->PreviewActorClass, Avatar->GetActorTransform(), Params);
		if (IsValid(PreviewActor))
		{
			PreviewActor->SetReplicates(false); PreviewActor->SetActorEnableCollision(false);
			PreviewCreated(PreviewActor);
		}
	}
	return bHoldingInput;
}

bool UNF_TmpActiveSkillBase::ConfirmLocalInput(FGameplayAbilityTargetDataHandle& OutTargetData)
{
	OutTargetData.Clear(); CheckInputState();
	if (!bHoldingInput) { return false; }
	const bool bValid = BuildTargetData(OutTargetData) && ValidateTargetData(OutTargetData);
	CancelLocalInput(); return bValid;
}

void UNF_TmpActiveSkillBase::CancelLocalInput()
{
	bHoldingInput = false;
	if (GetWorld()) { GetWorld()->GetTimerManager().ClearTimer(InputTimer); }
	auto* Previous = PreviewActor.Get(); PreviewActor = nullptr;
	if (IsValid(Previous)) { Previous->Destroy(); }
}

void UNF_TmpActiveSkillBase::CheckInputState()
{
	const auto* Info = GetCurrentActorInfo();
	if (bHoldingInput && (!Info || !Info->IsLocallyControlled() || IsActive() || !IsValid(GetAvatarActorFromActorInfo())
		|| !CanActivateAbility(CurrentSpecHandle, Info))) { CancelLocalInput(); }
}

bool UNF_TmpActiveSkillBase::BuildTargetData_Implementation(FGameplayAbilityTargetDataHandle& OutTargetData) const
{
	OutTargetData.Clear(); return true;
}
bool UNF_TmpActiveSkillBase::ValidateTargetData_Implementation(const FGameplayAbilityTargetDataHandle& TargetData) const
{
	// 방향/지점 검증은 별도 스킬이 명시적으로 구현하기 전까지 받지 않는다.
	return TargetData.Num() == 0;
}

void UNF_TmpActiveSkillBase::ActivateAbility(FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const auto* Data = GetActiveData();
	auto* Avatar = GetAvatarActorFromActorInfo();
	const FGameplayAbilityTargetDataHandle TargetData = TriggerEventData ? TriggerEventData->TargetData : FGameplayAbilityTargetDataHandle();
	if (!ActorInfo || !ActorInfo->IsNetAuthority() || !IsValid(Avatar) || !GetWorld() || !Data || IsStateBlocked()
		|| !ValidateTargetData(TargetData)) { EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo)) { EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }
	if (!IsActive() || IsStateBlocked()) { if (IsActive()) { EndAbility(Handle, ActorInfo, ActivationInfo, true, true); } return; }
	if (Data->ExecutionActorClass)
	{
		FActorSpawnParameters Params; Params.Owner = Avatar; Params.Instigator = Cast<APawn>(Avatar);
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		auto* Spawned = GetWorld()->SpawnActor<AActor>(Data->ExecutionActorClass, Avatar->GetActorTransform(), Params);
		if (!IsActive() || !IsValid(Spawned) || IsStateBlocked())
		{
			if (IsValid(Spawned)) { Spawned->Destroy(); }
			if (IsActive()) { EndAbility(Handle, ActorInfo, ActivationInfo, true, true); } return;
		}
		ExecutionActor = Spawned; bOwnsExecutionActor = true;
	}
	auto* SpawnedActor = ExecutionActor.Get();
	if (Data->bIndependentExecutionActor) { ExecutionActor = nullptr; bOwnsExecutionActor = false; }
	ExecuteSkill(SpawnedActor, TargetData);
	if (!IsActive()) { return; }
	if (Data->InputMethod != ENF_TmpSkillInput::Hold) { EndAbility(Handle, ActorInfo, ActivationInfo, true, false); }
}

void UNF_TmpActiveSkillBase::InputReleased(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);
	const auto* Data = GetActiveData();
	if (ActorInfo && ActorInfo->IsNetAuthority() && IsActive() && Data && Data->InputMethod == ENF_TmpSkillInput::Hold)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UNF_TmpActiveSkillBase::EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (!IsEndAbilityValid(Handle, ActorInfo)) { return; }
	if (ScopeLockCount > 0) { Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled); return; }
	CancelLocalInput();
	auto* Previous = ExecutionActor.Get(); const bool bDestroy = bOwnsExecutionActor;
	ExecutionActor = nullptr; bOwnsExecutionActor = false;
	if (bDestroy && IsValid(Previous)) { Previous->Destroy(); }
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UNF_TmpActiveSkillBase::HandleRestrictedState()
{
	if (IsStateBlocked()) { CancelLocalInput(); }
	Super::HandleRestrictedState();
}
void UNF_TmpActiveSkillBase::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	CancelLocalInput(); Super::OnRemoveAbility(ActorInfo, Spec);
}
void UNF_TmpActiveSkillBase::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	CancelLocalInput();
	if (IsActive()) { EndAbility(Spec.Handle, ActorInfo, CurrentActivationInfo, true, true); }
	Super::OnAvatarSet(ActorInfo, Spec);
}
