#include "GamePlay/Combat/Skill/Active/Movement/NFGA_Acceleration.h"

#include "Component/MotionMatchingComponent.h"
#include "Component/MotionMatchingCharacterMovementComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GamePlay/Combat/Skill/Execution/NF_AccelerationEffect.h"
#include "GamePlay/Combat/Skill/Execution/NF_SkillExecutionActor.h"
#include "TimerManager.h"
#include "Project_NF.h"

UNFGA_Acceleration::UNFGA_Acceleration()
{
	GetMutableSkillDefaults().SkillCategory = ENF_SkillCategory::Movement;
	GetMutableActiveDefaults().ExecutionMode = ENF_SkillExecutionMode::Timed;
	GetMutableActiveDefaults().ExecutionDuration = 5.0f;
	GetMutableSkillDefaults().BuildCost = 4;
}

bool UNFGA_Acceleration::HasValidSettings() const
{
	return !GetActiveSettings().ExecutionActorClass && !GetActiveSettings().bRequiresConfirmedTargetData
		&& GetSkillType() == ENF_SkillType::Active && GetSkillExecutionMode() == ENF_SkillExecutionMode::Timed
		&& NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerOnly
		&& FMath::IsFinite(SpeedMultiplier) && SpeedMultiplier >= 1.0f
		&& FMath::IsFinite(GetCastTime()) && GetCastTime() >= 0.0f
		&& FMath::IsFinite(GetExecutionDuration()) && GetExecutionDuration() > 0.0f;
}

bool UNFGA_Acceleration::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	ACharacter* Character = Cast<ACharacter>(Avatar);
	return IsValid(Character) && !Character->IsActorBeingDestroyed() && Character->GetCharacterMovement()
		&& HasValidSettings() && Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UNFGA_Acceleration::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!ActorInfo || !ActorInfo->IsNetAuthority() || !GetWorld() || !IsValid(Avatar)
		|| Avatar->IsActorBeingDestroyed() || !HasValidSettings() || !CanContinueSkillExecution())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return;
	}
	OriginalAvatar = Avatar;
	ACharacter* Character = Cast<ACharacter>(Avatar);
	if (!Character || !Character->GetCharacterMovement()) { EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }
	// Adding MotionMatchingComponent to a standard TPS character does not change its movement class.
	MovementComponent = Cast<UMotionMatchingCharacterMovementComponent>(Character->GetCharacterMovement())
		? Avatar->FindComponentByClass<UMotionMatchingComponent>() : nullptr;
	if (!MovementComponent.IsValid() && Avatar->FindComponentByClass<UMotionMatchingComponent>())
	{
		UE_LOG(LogProject_NF, Warning, TEXT("[Acceleration] %s uses standard movement with an added MotionMatchingComponent. Remove the extra component: its tick overwrites MaxWalkSpeed."), *Avatar->GetName());
	}
	Avatar->OnDestroyed.AddDynamic(this, &ThisClass::HandleAvatarDestroyed);
	if (GetCastTime() > 0.0f) { GetWorld()->GetTimerManager().SetTimer(CastTimer, this, &ThisClass::StartAcceleration, GetCastTime(), false); }
	else { StartAcceleration(); }
}

void UNFGA_Acceleration::StartAcceleration()
{
	if (!IsActive()) { return; }
	AActor* Avatar = OriginalAvatar.Get();
	UMotionMatchingComponent* Movement = MovementComponent.Get();
	ACharacter* Character = Cast<ACharacter>(Avatar);
	UCharacterMovementComponent* Standard = IsValid(Character) ? Character->GetCharacterMovement() : nullptr;
	if (!IsValid(Avatar) || Avatar->IsActorBeingDestroyed() || Avatar != GetAvatarActorFromActorInfo()
		|| (!IsValid(Movement) && !IsValid(Standard)) || !HasValidSettings() || !CanContinueSkillExecution())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true); return;
	}
	const float Current = Movement ? Movement->GetMovementSpeedMultiplier() : Standard->MaxWalkSpeed;
	if (!FMath::IsFinite(Current) || Current < 0.0f || !FMath::IsFinite(Current * SpeedMultiplier)
		|| !CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true); return;
	}
	// Commit can synchronously cancel the ability or apply other effects. Read the multiplier again.
	if (!IsActive()) { return; }
	if ((!IsValid(Movement) && !IsValid(Standard)) || !IsValid(Avatar) || Avatar->IsActorBeingDestroyed()
		|| Avatar != GetAvatarActorFromActorInfo() || !CanContinueSkillExecution())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true); return;
	}
	const float Updated = (Movement ? Movement->GetMovementSpeedMultiplier() : Standard->MaxWalkSpeed) * SpeedMultiplier;
	if (!FMath::IsFinite(Updated) || Updated < 0.0f)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true); return;
	}
	if (Movement)
	{
		AppliedMultiplier = SpeedMultiplier;
		bMultiplierApplied = true;
		Movement->SetMovementSpeedMultiplier(Updated);
	}
	else
	{
		FActorSpawnParameters Params; Params.Owner = Avatar;
		StandardMovementEffect = GetWorld()->SpawnActor<ANF_AccelerationEffect>(Params);
		if (!IsValid(StandardMovementEffect) || !StandardMovementEffect->Initialize(Character, SpeedMultiplier))
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true); return;
		}
	}
	Avatar->ForceNetUpdate();
	EndsAt = GetWorld()->GetTimeSeconds() + GetExecutionDuration();
	GetWorld()->GetTimerManager().SetTimer(LifetimeTimer, this, &ThisClass::CheckLifetime, 0.05f, true);
}

void UNFGA_Acceleration::CheckLifetime()
{
	if (!IsActive()) { return; }
	const bool bCancelled = !OriginalAvatar.IsValid() || OriginalAvatar->IsActorBeingDestroyed()
		|| OriginalAvatar.Get() != GetAvatarActorFromActorInfo()
		|| (!MovementComponent.IsValid() && !IsValid(StandardMovementEffect)) || !CanContinueSkillExecution();
	if (bCancelled || GetWorld()->GetTimeSeconds() >= EndsAt)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bCancelled);
	}
}

void UNFGA_Acceleration::HandleAvatarDestroyed(AActor* DestroyedActor)
{
	if (IsActive()) { EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true); }
}

void UNFGA_Acceleration::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (!IsEndAbilityValid(Handle, ActorInfo)) { return; }
	if (ScopeLockCount > 0) { Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled); return; }
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(CastTimer);
		GetWorld()->GetTimerManager().ClearTimer(LifetimeTimer);
	}
	if (IsValid(StandardMovementEffect)) { StandardMovementEffect->StopEffect(); }
	StandardMovementEffect = nullptr;
	if (bMultiplierApplied)
	{
		bMultiplierApplied = false; // Remove our factor only once, including reentrant cancellation.
		if (UMotionMatchingComponent* Movement = MovementComponent.Get())
		{
			const float Current = Movement->GetMovementSpeedMultiplier();
			if (FMath::IsFinite(Current) && ActorInfo && ActorInfo->IsNetAuthority())
			{
				Movement->SetMovementSpeedMultiplier(Current / AppliedMultiplier);
				if (AActor* Owner = Movement->GetOwner()) { Owner->ForceNetUpdate(); }
			}
		}
	}
	if (AActor* Avatar = OriginalAvatar.Get()) { Avatar->OnDestroyed.RemoveDynamic(this, &ThisClass::HandleAvatarDestroyed); }
	MovementComponent.Reset(); OriginalAvatar.Reset(); AppliedMultiplier = 1.0f;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
