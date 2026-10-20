#include "GamePlay/Combat/Skill/Active/Utility/NFGA_Grab.h"
#include "GamePlay/Combat/Skill/Execution/NF_SkillExecutionActor.h"
#include "GamePlay/Combat/Skill/Tasks/NFAbilityTask_PullCharacter.h"
#include "GamePlay/Combat/Components/NF_TargetQueryComponent.h"
#include "GamePlay/Combat/Projectiles/NF_GrabHook.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

UNFGA_Grab::UNFGA_Grab()
{
	SkillCategory = ENF_SkillCategory::Utility; BuildCost = 2;
	HookClass = ANF_GrabHook::StaticClass();
}

void UNFGA_Grab::SetGrabTarget(AActor* Target)
{
	const auto* Info = GetCurrentActorInfo();
	if (Info && Info->IsNetAuthority() && !IsActive()) { PendingTarget = Target; bHasPendingTarget = true; }
}

bool UNFGA_Grab::HasValidSettings() const
{
	return HookClass && HookClass->GetDefaultObject<ANF_GrabHook>()->HasValidSettings()
		&& !ExecutionActorClass && !bRequiresConfirmedTargetData && SkillType == ENF_SkillType::Active
		&& SkillExecutionMode == ENF_SkillExecutionMode::Instant && NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerOnly
		&& FMath::IsFinite(GrabRange) && GrabRange > 0.0f && FMath::IsFinite(GrabRadius) && GrabRadius > 0.0f
		&& FMath::IsFinite(StopDistance) && StopDistance > 0.0f && FMath::IsFinite(CastTime) && CastTime >= 0.0f && PullSettings.IsValid();
}

bool UNFGA_Grab::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	const auto* Character = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	return Character && Character->GetCharacterMovement()->IsActive() && Character->GetCharacterMovement()->MovementMode != MOVE_None
		&& HasValidSettings() && Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

bool UNFGA_Grab::ValidateTarget(ACharacter* Target, bool bCheckInitialAim) const
{
	AActor* Source = GetAvatarActorFromActorInfo();
	if (!IsValid(Source) || Target == Source || !ANF_CharacterPullActor::CanPull(Target, PullSettings)
		|| Target->GetWorld() != Source->GetWorld() || !UNF_TargetQueryComponent::PassesFilter(Target, Source, TargetFilter)
		|| (bCheckInitialAim && FVector::DistSquared(Source->GetActorLocation(), Target->GetActorLocation()) > FMath::Square(GrabRange))) { return false; }
	// At impact the swept projectile already proved the hit; pull movement still respects walls.
	if (!bCheckInitialAim) { return true; }
	FVector Eyes; FRotator Aim; Source->GetActorEyesViewPoint(Eyes, Aim);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NFGrabVisibility), false, Source); Params.AddIgnoredActor(Target);
	return !GetWorld()->LineTraceTestByChannel(Eyes, Target->GetActorLocation(), ObstacleChannel, Params);
}

bool UNFGA_Grab::CalculateDestination(ACharacter* Target, FVector& OutDestination) const
{
	const auto* Source = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Source || !Target) { return false; }
	FVector TowardTarget = Target->GetActorLocation() - Source->GetActorLocation(); TowardTarget.Z = 0.0f;
	if (!TowardTarget.Normalize()) { return false; }
	const UCapsuleComponent* SourceCapsule = Source->GetCapsuleComponent();
	const UCapsuleComponent* TargetCapsule = Target->GetCapsuleComponent();
	const UCharacterMovementComponent* Movement = Target->GetCharacterMovement();
	const float Separation = FMath::Max(StopDistance, SourceCapsule->GetScaledCapsuleRadius() + TargetCapsule->GetScaledCapsuleRadius() + 10.0f);
	const float Distance = FVector::Dist2D(Source->GetActorLocation(), Target->GetActorLocation());
	const float HalfHeight = TargetCapsule->GetScaledCapsuleHalfHeight();
	const float SourceFeet = Source->GetActorLocation().Z - SourceCapsule->GetScaledCapsuleHalfHeight();
	const FCollisionShape Capsule = FCollisionShape::MakeCapsule(TargetCapsule->GetScaledCapsuleRadius(), HalfHeight);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NFGrabDestination), false, Target);
	// Prefer the requested stop distance; try nearby positions farther from the caster if occupied.
	for (const float Offset : { 0.0f, 30.0f, 60.0f })
	{
		if (Distance <= Separation + Offset + PullSettings.ArrivalTolerance) { continue; }
		FVector Candidate = Source->GetActorLocation() + TowardTarget * (Separation + Offset);
		Candidate.Z = SourceFeet + HalfHeight + 2.0f;
		const float SearchUp = FMath::Max(50.0f, Movement->MaxStepHeight + 10.0f);
		const float SearchDown = FMath::Max(200.0f, HalfHeight * 2.0f);
		FHitResult Floor;
		if (GetWorld()->SweepSingleByProfile(Floor, Candidate + FVector(0, 0, SearchUp), Candidate - FVector(0, 0, SearchDown),
			Target->GetActorQuat(), TargetCapsule->GetCollisionProfileName(), Capsule, Params))
		{
			if (Floor.bStartPenetrating || !Movement->IsWalkable(Floor)) { continue; }
			Candidate = Floor.Location + FVector(0, 0, 2.0f);
		}
		else if (Movement->IsMovingOnGround()) { continue; }
		// Airborne targets may be pulled through open space, without requiring a floor below them.
		if (!GetWorld()->OverlapBlockingTestByProfile(Candidate, Target->GetActorQuat(), TargetCapsule->GetCollisionProfileName(), Capsule, Params))
		{
			OutDestination = Candidate; return true;
		}
	}
	return false;
}
void UNFGA_Grab::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AActor* ExplicitTarget = TriggerEventData && TriggerEventData->Target ? const_cast<AActor*>(TriggerEventData->Target.Get()) : PendingTarget.Get();
	const bool bExplicitTarget = (TriggerEventData && TriggerEventData->Target) || bHasPendingTarget;
	PendingTarget.Reset(); bHasPendingTarget = false;
	if (!ActorInfo || !ActorInfo->IsNetAuthority() || !HasValidSettings() || !CanContinueSkillExecution())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return;
	}
	bUseExplicitAim = bExplicitTarget;
	ActiveTarget = Cast<ACharacter>(ExplicitTarget);
	OriginalAvatar = GetAvatarActorFromActorInfo();
	if (bUseExplicitAim && !ValidateTarget(ActiveTarget.Get())) { EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }
	if (CastTime > 0.0f) { GetWorld()->GetTimerManager().SetTimer(GrabCastTimer, this, &ThisClass::StartGrab, CastTime, false); }
	else { StartGrab(); }
}

void UNFGA_Grab::StartGrab()
{
	if (!CanContinueHook() || !HasValidSettings() || (bUseExplicitAim && !ValidateTarget(ActiveTarget.Get())))
	{
		if (IsActive()) { EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true); } return;
	}
	AActor* Source = OriginalAvatar.Get();
	const FVector Origin = HookClass->GetDefaultObject<ANF_GrabHook>()->GetSourceAnchor(Source);
	FVector Eyes; FRotator Aim; Source->GetActorEyesViewPoint(Eyes, Aim);
	const FVector AimPoint = bUseExplicitAim ? ActiveTarget->GetActorLocation() : Eyes + Aim.Vector() * GrabRange;
	const FVector Direction = (AimPoint - Origin).GetSafeNormal();
	if (Origin.ContainsNaN() || Direction.IsNearlyZero()) { EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true); return; }
	const FTransform Transform(Direction.Rotation(), Origin);
	ActiveHook = GetWorld()->SpawnActorDeferred<ANF_GrabHook>(HookClass, Transform, Source, Cast<APawn>(Source), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!ActiveHook) { EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true); return; }
	ActiveHook->FinishSpawning(Transform);
	if (!IsValid(ActiveHook)) { EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true); return; }
	ActiveHook->OnCharacterHit.BindUObject(this, &ThisClass::HandleHookHit);
	ActiveHook->CanContinue.BindUObject(this, &ThisClass::CanContinueHook);
	ActiveHook->OnFlightEnded.AddUObject(this, &ThisClass::HandleHookFlightEnded);
	// Cost/cooldown start at launch, including shots which miss.
	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true); return;
	}
	if (!IsActive()) { return; }
	if (!IsValid(ActiveHook) || !ActiveHook->StartFlight(Source, Direction, GrabRange, GrabRadius, PullSettings.MaxDuration))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

bool UNFGA_Grab::CanContinueHook() const
{
	const auto* Source = Cast<ACharacter>(OriginalAvatar.Get());
	return IsActive() && IsValid(Source) && !Source->IsActorBeingDestroyed() && Source == GetAvatarActorFromActorInfo()
		&& Source->GetCharacterMovement()->IsActive() && Source->GetCharacterMovement()->MovementMode != MOVE_None && CanContinueSkillExecution();
}

bool UNFGA_Grab::HandleHookHit(ACharacter* Target)
{
	FVector Destination;
	if (!CanContinueHook() || !IsValid(ActiveHook) || !ValidateTarget(Target, false))
	{
		UE_LOG(LogTemp, Display, TEXT("[Grab] Hit rejected: invalid/immune/busy target %s"), *GetNameSafe(Target));
		return false;
	}
	if (!CalculateDestination(Target, Destination))
	{
		UE_LOG(LogTemp, Display, TEXT("[Grab] Hit rejected: no safe destination or target already close (%s)"), *GetNameSafe(Target));
		return false;
	}
	ActiveTarget = Target;
	UNFAbilityTask_PullCharacter* Task = UNFAbilityTask_PullCharacter::PullCharacter(this, Target, Destination, PullSettings);
	PullTask = Task;
	Task->OnFinished.AddDynamic(this, &ThisClass::HandlePullFinished);
	Task->ReadyForActivation();
	// Activation can fail synchronously and finish the GA. Never display an unconfirmed attachment.
	if (PullTask != Task || !Task->HasStartedPull() || !IsActive() || !IsValid(ActiveHook)) { return false; }
	ActiveHook->Latch(Target);
	return true;
}

void UNFGA_Grab::HandlePullFinished(bool bReachedDestination)
{
	UE_LOG(LogTemp, Display, TEXT("[Grab] Pull finished: Reached=%d Result=%s"), bReachedDestination,
		PullTask ? *UEnum::GetValueAsString(PullTask->Result) : TEXT("Cancelled"));
	PullTask = nullptr;
	if (IsValid(ActiveHook))
	{
		// Simulation cleanup / cosmetic retraction must not keep the gameplay ability alive.
		ActiveHook->OnCharacterHit.Unbind(); ActiveHook->CanContinue.Unbind(); ActiveHook->OnFlightEnded.RemoveAll(this);
		ActiveHook->BeginReturn(); ActiveHook = nullptr;
	}
	if (IsActive()) { EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, !bReachedDestination); }
}

void UNFGA_Grab::HandleHookFlightEnded()
{
	// Losing a hook or its presentation after confirmed impact never cancels an ongoing pull.
	if (PullTask) { return; }
	if (IsValid(ActiveHook))
	{
		ActiveHook->OnCharacterHit.Unbind(); ActiveHook->CanContinue.Unbind(); ActiveHook->OnFlightEnded.RemoveAll(this);
		ActiveHook = nullptr;
	}
	if (IsActive()) { EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false); }
}
void UNFGA_Grab::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (!IsEndAbilityValid(Handle, ActorInfo)) { return; }
	if (ScopeLockCount > 0) { Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled); return; }
	if (GetWorld()) { GetWorld()->GetTimerManager().ClearTimer(GrabCastTimer); }
	if (PullTask) { PullTask->OnFinished.RemoveAll(this); PullTask->EndTask(); PullTask = nullptr; }
	if (IsValid(ActiveHook))
	{
		ActiveHook->OnCharacterHit.Unbind(); ActiveHook->CanContinue.Unbind();
		ActiveHook->OnFlightEnded.RemoveAll(this); ActiveHook->Abort();
	}
	ActiveHook = nullptr; bUseExplicitAim = false;
	ActiveTarget.Reset(); OriginalAvatar.Reset();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
