#include "GamePlay/Combat/Skill/Execution/NF_CharacterPullActor.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/RootMotionSource.h"
#include "Net/UnrealNetwork.h"
#include "EngineUtils.h"

bool FNF_CharacterPullSettings::IsValid() const
{
	return FMath::IsFinite(Speed) && Speed > 0.0f && FMath::IsFinite(MaxDuration) && MaxDuration >= 0.05f
		&& FMath::IsFinite(ArrivalTolerance) && ArrivalTolerance > 0.0f
		&& (!StunEffectClass || StunEffectClass->GetDefaultObject<UGameplayEffect>()->DurationPolicy != EGameplayEffectDurationType::Instant);
}

ANF_CharacterPullActor::ANF_CharacterPullActor()
{
	bReplicates = true;
	bOnlyRelevantToOwner = true;
	SetNetUpdateFrequency(60.0f);
	PrimaryActorTick.bCanEverTick = true;
}

bool ANF_CharacterPullActor::CanPull(ACharacter* Target, const FNF_CharacterPullSettings& Settings)
{
	if (!IsValid(Target) || Target->IsActorBeingDestroyed() || !Settings.IsValid()) { return false; }
	UCharacterMovementComponent* Movement = Target->GetCharacterMovement();
	if (!Movement || !Movement->IsActive() || Movement->MovementMode == MOVE_None) { return false; }
	for (TActorIterator<ANF_CharacterPullActor> It(Target->GetWorld()); It; ++It)
	{
		if (It->Motion.Target == Target && It->bStarted && !It->bStopping) { return false; }
	}
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	return (!Settings.StunEffectClass || ASC) && (!ASC ||
		(!ASC->HasAnyMatchingGameplayTags(Settings.ImmunityTags) && !ASC->HasAnyMatchingGameplayTags(Settings.InterruptTags)));
}

bool ANF_CharacterPullActor::StartPull(AActor* Source, ACharacter* Target, FVector Destination,
	const FNF_CharacterPullSettings& Settings, UAbilitySystemComponent* SourceASC, float AbilityLevel)
{
	if (!HasAuthority() || bStarted || !IsValid(Source) || Source == Target || Source->GetWorld() != GetWorld()
		|| !CanPull(Target, Settings) || Target->GetWorld() != GetWorld() || Destination.ContainsNaN()
		|| (Settings.StunEffectClass && !SourceASC)) { Result = ENF_CharacterPullResult::InvalidTarget; return false; }
	SourceActor = Source; PullSettings = Settings; Motion.Target = Target; Motion.Destination = Destination.GridSnap(1.0);
	Motion.StartLocation = Target->GetActorLocation().GridSnap(1.0);
	Motion.MotionName = FName(*FGuid::NewGuid().ToString(EGuidFormats::Digits));
	TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	SetOwner(Target); SetActorLocation(Target->GetActorLocation());
	const float Distance = FVector::Distance(Target->GetActorLocation(), Destination);
	Motion.Duration = FMath::Max(Distance / Settings.Speed, 0.05f);
	Motion.ActiveDuration = FMath::Min(Motion.Duration, Settings.MaxDuration);
	const AGameStateBase* State = GetWorld()->GetGameState();
	Motion.StartServerTime = State ? State->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
	// Mark initialized before GE callbacks, so synchronous destruction can still clean up safely.
	bStarted = true;
	// Reserve this target before effect callbacks can attempt another pull.
	ApplyMotion(Motion.Duration);
	if (MotionId == MAX_uint16) { StopPull(false, ENF_CharacterPullResult::MovementRejected); return false; }
	if (Settings.StunEffectClass)
	{
		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext(); Context.AddSourceObject(Source);
		FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(Settings.StunEffectClass, AbilityLevel, Context);
		if (!Spec.IsValid()) { StopPull(false, ENF_CharacterPullResult::EffectRejected); return false; }
		// Every grab owns one independent GE handle; shared stacking effects cannot be safely removed.
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		const bool bSharedStack = Spec.Data->Def->StackingType != EGameplayEffectStackingType::None;
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		if (bSharedStack) { StopPull(false, ENF_CharacterPullResult::EffectRejected); return false; }
		Spec.Data->SetDuration(Motion.ActiveDuration + 0.25f, true);
		StunHandle = SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC.Get());
		if (!StunHandle.IsValid() || bStopping || !IsValid(Target))
		{
			if (TargetASC.IsValid() && StunHandle.IsValid()) { TargetASC->RemoveActiveGameplayEffect(StunHandle); }
			StunHandle.Invalidate(); StopPull(false, ENF_CharacterPullResult::EffectRejected); return false;
		}
	}
	Motion.bActive = true; EndsAt = GetWorld()->GetTimeSeconds() + Motion.ActiveDuration;
	Target->OnActorHit.AddUniqueDynamic(this, &ThisClass::HandleHit);
	ForceNetUpdate(); Target->ForceNetUpdate();
	return true;
}

void ANF_CharacterPullActor::ApplyMotion(float Duration, float Elapsed)
{
	if (!IsValid(Motion.Target) || MotionId != MAX_uint16 || Duration <= 0.0f) { return; }
	UCharacterMovementComponent* Movement = Motion.Target->GetCharacterMovement();
	// A server movement correction may have installed this command before the actor RepNotify.
	if (const auto Existing = Movement->GetRootMotionSource(Motion.MotionName))
	{
		MotionId = Existing->LocalID; return;
	}
	auto Root = MakeShared<FRootMotionSource_MoveToForce>();
	Root->InstanceName = Motion.MotionName; Root->AccumulateMode = ERootMotionAccumulateMode::Override;
	Root->Priority = 1000; Root->StartLocation = Motion.StartLocation; Root->TargetLocation = Motion.Destination;
	Root->Duration = Duration; Root->bRestrictSpeedToExpected = true;
	Root->Settings.SetFlag(ERootMotionSourceSettingsFlags::UseSensitiveLiftoffCheck);
	Root->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	Root->FinishVelocityParams.SetVelocity = FVector::ZeroVector;
	Root->SetTime(Elapsed);
	MotionId = Movement->ApplyRootMotionSource(Root);
}

void ANF_CharacterPullActor::OnRep_Motion()
{
	if (!Motion.bActive) { RemoveMotion(); return; }
	if (!IsValid(Motion.Target) || !Motion.Target->IsLocallyControlled()) { return; }
	const AGameStateBase* State = GetWorld()->GetGameState();
	// Do not restart an old command at t=0 before the shared server clock is available.
	if (!State) { return; }
	const float Elapsed = static_cast<float>(FMath::Max(0.0, State->GetServerWorldTimeSeconds() - Motion.StartServerTime));
	const float Remaining = Motion.ActiveDuration - Elapsed;
	if (Remaining <= 0.0f) { RemoveMotion(); return; }
	ApplyMotion(Motion.Duration, Elapsed);
	EndsAt = GetWorld()->GetTimeSeconds() + Remaining;
}

void ANF_CharacterPullActor::RemoveMotion()
{
	if (IsValid(Motion.Target) && !Motion.MotionName.IsNone() &&
		(MotionId != MAX_uint16 || Motion.Target->GetCharacterMovement()->GetRootMotionSource(Motion.MotionName)))
	{
		// Corrections may remap local IDs; the unique name also removes a corrected source.
		Motion.Target->GetCharacterMovement()->RemoveRootMotionSource(Motion.MotionName);
		Motion.Target->GetCharacterMovement()->StopMovementImmediately();
		Motion.Target->OnActorHit.RemoveDynamic(this, &ThisClass::HandleHit);
	}
	MotionId = MAX_uint16;
}

void ANF_CharacterPullActor::HandleHit(AActor* SelfActor, AActor* OtherActor, FVector Impulse, const FHitResult& Hit)
{
	if (!HasAuthority() || !Motion.bActive || !IsValid(Motion.Target)) { return; }
	const FVector Direction = (FVector(Motion.Destination) - Motion.Target->GetActorLocation()).GetSafeNormal();
	const UCharacterMovementComponent* Movement = Motion.Target->GetCharacterMovement();
	if (OtherActor == this || Movement->IsWalkable(Hit)) { return; }
	if (Hit.bBlockingHit && FVector::DotProduct(Direction, Hit.ImpactNormal) < -0.1f) { StopPull(false, ENF_CharacterPullResult::Blocked); }
}

void ANF_CharacterPullActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority())
	{
		// Retry while actor references / local possession are not ready at the first RepNotify.
		if (Motion.bActive && MotionId == MAX_uint16) { OnRep_Motion(); }
		if (MotionId != MAX_uint16 && GetWorld()->GetTimeSeconds() >= EndsAt) { RemoveMotion(); }
		return;
	}
	if (!Motion.bActive) { return; }
	const ACharacter* SourceCharacter = Cast<ACharacter>(SourceActor.Get());
	if (!SourceActor.IsValid() || SourceActor->IsActorBeingDestroyed() || !IsValid(Motion.Target) || Motion.Target->IsActorBeingDestroyed()
		|| !Motion.Target->GetCharacterMovement()->IsActive() || Motion.Target->GetCharacterMovement()->MovementMode == MOVE_None
		|| (SourceCharacter && SourceCharacter->GetCharacterMovement()->MovementMode == MOVE_None))
	{
		StopPull(false, ENF_CharacterPullResult::InvalidTarget); return;
	}
	if (TargetASC.IsValid() && (TargetASC->HasAnyMatchingGameplayTags(PullSettings.InterruptTags)
		|| TargetASC->HasAnyMatchingGameplayTags(PullSettings.ImmunityTags)))
	{
		StopPull(false, ENF_CharacterPullResult::ImmuneOrInterrupted); return;
	}
	const bool bReached = FVector::DistSquared(Motion.Target->GetActorLocation(), Motion.Destination) <= FMath::Square(PullSettings.ArrivalTolerance);
	if (bReached || GetWorld()->GetTimeSeconds() >= EndsAt) { StopPull(bReached, ENF_CharacterPullResult::TimedOut); }
}

void ANF_CharacterPullActor::StopPull(bool bReached, ENF_CharacterPullResult Reason)
{
	if (!HasAuthority() || bStopping) { return; }
	bStopping = true; Motion.bActive = false;
	Result = bReached ? ENF_CharacterPullResult::Reached : Reason;
	UE_LOG(LogTemp, Display, TEXT("[GrabPull] Target=%s Result=%s"), *GetNameSafe(Motion.Target), *UEnum::GetValueAsString(Result));
	RemoveMotion();
	if (TargetASC.IsValid() && StunHandle.IsValid()) { TargetASC->RemoveActiveGameplayEffect(StunHandle); }
	StunHandle.Invalidate();
	ForceNetUpdate();
	OnPullEnded.Broadcast(bReached);
	SetLifeSpan(0.2f);
}

void ANF_CharacterPullActor::EndPlay(const EEndPlayReason::Type Reason)
{
	if (HasAuthority()) { StopPull(); }
	else { RemoveMotion(); }
	Super::EndPlay(Reason);
}

void ANF_CharacterPullActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANF_CharacterPullActor, Motion);
}
