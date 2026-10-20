#include "GamePlay/Combat/Skill/Active/Movement/NFGA_Movement.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GamePlay/Combat/Skill/Execution/NF_SkillExecutionActor.h"
#include "Engine/World.h"
#include "TimerManager.h"

UNFGA_Movement::UNFGA_Movement()
{
	SkillCategory = ENF_SkillCategory::Movement;
	BuildCost = 4;
}

void UNFGA_Movement::SetMovementTarget(AActor* Target)
{
	const FGameplayAbilityActorInfo* Info = GetCurrentActorInfo();
	if (Info && Info->IsNetAuthority() && !IsActive()) { PendingTarget = Target; }
}

bool UNFGA_Movement::HasValidMovementSettings() const
{
	return !ExecutionActorClass && !bRequiresConfirmedTargetData
		&& NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerOnly
		&& SkillType == ENF_SkillType::Active && SkillExecutionMode == ENF_SkillExecutionMode::Instant
		&& !DirectionAxis.ContainsNaN() && !DirectionAxis.IsNearlyZero()
		&& FMath::IsFinite(MovementSpeed) && MovementSpeed > 0.0f
		&& FMath::IsFinite(CastTime) && CastTime >= 0.0f
		&& (MovementMethod == ENF_MovementMethod::Launch ||
			(FMath::IsFinite(MovementDuration) && MovementDuration > 0.0f));
}

bool UNFGA_Movement::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	const ACharacter* Character = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	return Character && Character->GetCharacterMovement() && Character->GetCharacterMovement()->IsActive()
		&& Character->GetCharacterMovement()->MovementMode != MOVE_None
		&& HasValidMovementSettings() && Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

bool UNFGA_Movement::ResolveDirection(ACharacter* Character, const AActor* Target, FVector& OutDirection) const
{
	FRotator Basis = FRotator::ZeroRotator;
	switch (DirectionSource)
	{
	case ENF_MovementDirection::Avatar: Basis = Character->GetActorRotation(); break;
	case ENF_MovementDirection::Aim:
		Basis = Character->GetBaseAimRotation(); break;
	case ENF_MovementDirection::Target:
	{
		if (!IsValid(Target) || Target == Character || Target->GetWorld() != Character->GetWorld()) { return false; }
		FVector ToTarget = Target->GetActorLocation() - Character->GetActorLocation();
		if (bHorizontalOnly) { ToTarget.Z = 0.0f; }
		if (ToTarget.IsNearlyZero()) { return false; }
		Basis = ToTarget.Rotation(); break;
	}
	default: break;
	}
	if (bHorizontalOnly) { Basis.Pitch = 0.0f; Basis.Roll = 0.0f; }
	OutDirection = Basis.RotateVector(DirectionAxis);
	if (bHorizontalOnly) { OutDirection.Z = 0.0f; }
	return !OutDirection.ContainsNaN() && OutDirection.Normalize();
}

void UNFGA_Movement::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ACharacter* Character = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	const AActor* Target = TriggerEventData && TriggerEventData->Target ? TriggerEventData->Target.Get() : PendingTarget.Get();
	PendingTarget.Reset();
	if (!ActorInfo || !ActorInfo->IsNetAuthority() || !Character || !HasValidMovementSettings()
		|| !CanContinueSkillExecution() || !ResolveDirection(Character, Target, ActiveDirection))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return;
	}
	MovingCharacter = Character;
	if (CastTime > 0.0f) { GetWorld()->GetTimerManager().SetTimer(MovementCastTimer, this, &ThisClass::StartMovement, CastTime, false); }
	else { StartMovement(); }
}

void UNFGA_Movement::StartMovement()
{
	ACharacter* Character = MovingCharacter.Get();
	if (!IsActive() || !IsValid(Character) || Character != GetAvatarActorFromActorInfo()
		|| !Character->GetCharacterMovement()->IsActive() || Character->GetCharacterMovement()->MovementMode == MOVE_None || !CanContinueSkillExecution()
		|| !CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		if (IsActive()) { EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true); }
		return;
	}
	if (!IsActive()) { return; }
	if (MovementMethod == ENF_MovementMethod::Launch)
	{
		Character->LaunchCharacter(ActiveDirection * MovementSpeed, bOverrideLaunchXY, bOverrideLaunchZ);
		FinishMovement(); return;
	}
	// A single velocity-change impulse keeps walking characters grounded and leaves gravity,
	// friction and collision response to CharacterMovement. No root motion or position writes.
	bDashStarted = true;
	Character->GetCharacterMovement()->AddImpulse(ActiveDirection * MovementSpeed, true);
	if (bStopOnBlockingHit) { Character->OnActorHit.AddUniqueDynamic(this, &ThisClass::HandleBlockingHit); }
	GetWorld()->GetTimerManager().SetTimer(MovementEndTimer, this, &ThisClass::FinishMovement, MovementDuration, false);
}

void UNFGA_Movement::HandleBlockingHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit)
{
	// Floor contacts must not end a horizontal dash.
	if (IsActive() && Hit.bBlockingHit && FVector::DotProduct(ActiveDirection, Hit.ImpactNormal) < -0.1f) { FinishMovement(); }
}

void UNFGA_Movement::FinishMovement()
{
	if (IsActive()) { EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false); }
}

void UNFGA_Movement::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (!IsEndAbilityValid(Handle, ActorInfo)) { return; }
	if (ScopeLockCount > 0) { Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled); return; }
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(MovementCastTimer);
		GetWorld()->GetTimerManager().ClearTimer(MovementEndTimer);
	}
	if (ACharacter* Character = MovingCharacter.Get())
	{
		Character->OnActorHit.RemoveDynamic(this, &ThisClass::HandleBlockingHit);
		if (bDashStarted && bStopVelocityOnEnd)
		{
			// Explicit hard-stop also clears an impulse queued before the first movement tick.
			Character->GetCharacterMovement()->ClearAccumulatedForces();
			Character->GetCharacterMovement()->StopMovementImmediately();
		}
	}
	bDashStarted = false;
	MovingCharacter.Reset(); ActiveDirection = FVector::ZeroVector;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
