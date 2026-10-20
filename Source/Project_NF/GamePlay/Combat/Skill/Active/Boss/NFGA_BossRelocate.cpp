#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossRelocate.h"

#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GamePlay/Combat/Boss/NF_BossBase.h"
#include "GamePlay/Combat/Boss/NF_BossCombatEffects.h"
#include "GAS/Tags/NFGameplayCueTags.h"
#include "GAS/Tags/NFTag_BossCombat.h"
#include "GameplayEffect.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "Player/Character/NF_Player.h"
#include "TimerManager.h"

UNFGA_BossRelocate::UNFGA_BossRelocate()
{
	ExecutionDuration = 7.45f;
	DamageEffectClass = UNFGE_BossDamageToPlayer::StaticClass();
	DamageMagnitudeTag = NFTag_BossCombat::Data_Damage;
	AirborneEffectClass = UNFGE_BossAirborne::StaticClass();
}

void UNFGA_BossRelocate::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ANF_BossBase* Boss = GetBoss();
	if (!Boss || !ActorInfo || !ActorInfo->IsNetAuthority() || !CommitAbility(Handle, ActorInfo, ActivationInfo)
		|| !ChooseEmergencePoint(EmergencePoint))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	StartingLocation = Boss->GetActorLocation();
	FVector StartingGroundPoint;
	StartingHoverHeight = FindGroundPoint(Boss, StartingGroundPoint)
		? FMath::Max(StartingLocation.Z - StartingGroundPoint.Z, 0.0f)
		: 0.0f;
	ExecutionDuration = FMath::Max(DirtCueDuration, 0.0f)
		+ FMath::Max(AscentDuration, 0.0f)
		+ FMath::Max(HiddenDurationBeforeWarning, 0.0f)
		+ FMath::Max(EmergenceWarningDuration, 0.0f)
		+ FMath::Max(DiveDuration, 0.0f)
		+ FMath::Max(AscentDuration, 0.0f)
		+ 0.1f;
	ExecuteBossCue(NFGameplayCueTags::GameplayCue_Boss_TargetWarning,
		Boss->GetActorLocation() + FVector(0.0f, 0.0f, 250.0f));
	GetWorld()->GetTimerManager().SetTimer(TakeoffTimer, this, &UNFGA_BossRelocate::BeginTakeoff,
		FMath::Max(DirtCueDuration, 0.0f), false);
}

void UNFGA_BossRelocate::BeginTakeoff()
{
	ANF_BossBase* Boss = GetBoss();
	if (!IsActive() || !Boss || !Boss->HasAuthority())
	{
		return;
	}
	bIsInFlight = true;
	Boss->SetFlying(true);
	const FVector AirborneLocation = StartingLocation + FVector::UpVector * (FMath::Max(FlightAltitude, 0.0f) * 3.0f);
	StartBossMovement(AirborneLocation, AscentDuration, EMovementPhase::TakingOff);
}

void UNFGA_BossRelocate::StartBossMovement(const FVector& Destination, const float Duration,
	const EMovementPhase Phase)
{
	ANF_BossBase* Boss = GetBoss();
	UWorld* World = Boss ? Boss->GetWorld() : nullptr;
	if (!IsActive() || !Boss || !World || !Boss->HasAuthority())
	{
		if (IsActive()) EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	MovementStart = Boss->GetActorLocation();
	MovementDestination = Destination;
	CurrentMovementDuration = FMath::Max(Duration, 0.0f);
	MovementStartTime = World->GetTimeSeconds();
	MovementPhase = Phase;
	if (CurrentMovementDuration <= UE_KINDA_SMALL_NUMBER)
	{
		Boss->SetActorLocation(MovementDestination, false, nullptr, ETeleportType::TeleportPhysics);
		FinishBossMovement();
		return;
	}

	World->GetTimerManager().SetTimer(MovementTimer, this, &UNFGA_BossRelocate::UpdateBossMovement, 1.0f / 60.0f, true);
}

void UNFGA_BossRelocate::UpdateBossMovement()
{
	ANF_BossBase* Boss = GetBoss();
	UWorld* World = Boss ? Boss->GetWorld() : nullptr;
	if (!IsActive() || !Boss || !World || !Boss->HasAuthority())
	{
		if (IsActive()) EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	const float Alpha = FMath::Clamp((World->GetTimeSeconds() - MovementStartTime)
		/ FMath::Max(CurrentMovementDuration, UE_KINDA_SMALL_NUMBER), 0.0f, 1.0f);
	const float CurvedAlpha = MovementPhase == EMovementPhase::Diving
		? FMath::InterpEaseIn(0.0f, 1.0f, Alpha, 5.0f)
		: FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.0f);
	Boss->SetActorLocation(FMath::Lerp(MovementStart, MovementDestination, CurvedAlpha));

	if (Alpha >= 1.0f)
	{
		Boss->SetActorLocation(MovementDestination, false, nullptr, ETeleportType::TeleportPhysics);
		World->GetTimerManager().ClearTimer(MovementTimer);
		FinishBossMovement();
	}
}

void UNFGA_BossRelocate::FinishBossMovement()
{
	const EMovementPhase FinishedPhase = MovementPhase;
	MovementPhase = EMovementPhase::None;
	if (FinishedPhase == EMovementPhase::TakingOff)
	{
		GetWorld()->GetTimerManager().SetTimer(LandingWarningTimer, this,
			&UNFGA_BossRelocate::ShowEmergenceWarning, FMath::Max(HiddenDurationBeforeWarning, 0.0f), false);
	}
	else if (FinishedPhase == EMovementPhase::Diving)
	{
		Emerge();
	}
	else if (FinishedPhase == EMovementPhase::ReturningToHover)
	{
		if (ANF_BossBase* Boss = GetBoss())
		{
			Boss->SetFlying(false);
		}
		bIsInFlight = false;
		FinishPattern();
	}
}

void UNFGA_BossRelocate::ShowEmergenceWarning()
{
	if (!IsActive()) return;
	ShowTelegraph(EmergencePoint, EmergenceRadius, EmergenceWarningDuration);
	GetWorld()->GetTimerManager().SetTimer(DiveTimer, this, &UNFGA_BossRelocate::BeginDive,
		FMath::Max(EmergenceWarningDuration, 0.0f), false);
}

void UNFGA_BossRelocate::BeginDive()
{
	if (!IsActive()) return;
	StartBossMovement(EmergencePoint, DiveDuration, EMovementPhase::Diving);
}

void UNFGA_BossRelocate::Emerge()
{
	ANF_BossBase* Boss = GetBoss();
	if (!IsActive() || !Boss || !Boss->HasAuthority())
	{
		return;
	}
	// Resolve the floor beneath the boss again at arrival so both the landing effects
	// and their GameplayCue use the actual ground location, even on uneven terrain.
	FVector LandingGroundPoint = EmergencePoint;
	if (FindGroundPoint(Boss, LandingGroundPoint))
	{
		EmergencePoint = LandingGroundPoint;
		Boss->SetActorLocation(EmergencePoint, false, nullptr, ETeleportType::TeleportPhysics);
	}
	ExecuteBossCue(NFGameplayCueTags::GameplayCue_Boss_Emerge, LandingGroundPoint);
	ExecuteBossCue(NFGameplayCueTags::GameplayCue_Boss_Relocate_Landing, LandingGroundPoint);

	const FGameplayEffectSpecHandle DamageSpec = MakeBossEffectSpec(DamageEffectClass, DamageMagnitudeTag,
		Boss->GetPatternDamage(ENF_BossPatternId::Relocate, Damage));
	const FGameplayEffectSpecHandle AirborneSpec = MakeBossEffectSpec(AirborneEffectClass, FGameplayTag(), 0.0f);
	for (ANF_Player* Player : FindPlayersInRange(EmergencePoint, EmergenceRadius))
	{
		ApplyEffectToActor(Player, DamageSpec);
		ApplyEffectToActor(Player, AirborneSpec);
		Player->LaunchCharacter(FVector(0.0f, 0.0f, AirborneLaunchVelocity), true, true);
	}
	// After the floor impact, return to the same hover height measured at takeoff.
	const FVector HoverLocation = LandingGroundPoint + FVector::UpVector * StartingHoverHeight;
	StartBossMovement(HoverLocation, AscentDuration, EMovementPhase::ReturningToHover);
}

void UNFGA_BossRelocate::FinishPattern()
{
	if (IsActive()) EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

bool UNFGA_BossRelocate::ChooseEmergencePoint(FVector& OutPoint) const
{
	ANF_BossBase* Boss = GetBoss();
	if (!Boss)
	{
		return false;
	}
	const TArray<ANF_Player*> Players = FindPlayersInRange(Boss->GetActorLocation(), RelocationRange);
	if (Players.IsEmpty())
	{
		OutPoint = Boss->GetActorLocation();
		return true;
	}
	ANF_Player* Target = Players[FMath::RandRange(0, Players.Num() - 1)];
	FVector RandomDirection = FMath::VRand();
	RandomDirection.Z = 0.0f;
	const FVector Offset = RandomDirection.GetSafeNormal2D() * FMath::FRandRange(700.0f, 1200.0f);
	const FVector Candidate = Target->GetActorLocation() + Offset;
	UWorld* World = Boss->GetWorld();
	if (!World)
	{
		return false;
	}
	const FVector TraceStart = Candidate + FVector(0.0f, 0.0f, 5000.0f);
	const FVector TraceEnd = Candidate - FVector(0.0f, 0.0f, 10000.0f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NFBossEmergenceGround), false, Boss);
	FHitResult Hit;
	if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
	{
		OutPoint = Hit.ImpactPoint;
	}
	else if (!FindGroundPoint(Target, OutPoint))
	{
		return false;
	}
	return true;
}

void UNFGA_BossRelocate::ExecuteBossCue(const FGameplayTag CueTag, const FVector& Location) const
{
	if (ANF_BossBase* Boss = GetBoss())
	{
		if (UAbilitySystemComponent* BossASC = Boss->GetAbilitySystemComponent())
		{
			FGameplayCueParameters Parameters;
			Parameters.Location = Location;
			Parameters.EffectCauser = Boss;
			BossASC->ExecuteGameplayCue(CueTag, Parameters);
		}
	}
}

void UNFGA_BossRelocate::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility, const bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		FTimerManager& Timers = World->GetTimerManager();
		Timers.ClearTimer(TakeoffTimer);
		Timers.ClearTimer(LandingWarningTimer);
		Timers.ClearTimer(DiveTimer);
		Timers.ClearTimer(FinishTimer);
		Timers.ClearTimer(MovementTimer);
	}
	if (bWasCancelled)
	{
		if (bIsInFlight)
		{
			if (ANF_BossBase* Boss = GetBoss(); IsValid(Boss) && Boss->HasAuthority())
			{
				Boss->SetFlying(false);
				Boss->SetActorLocation(StartingLocation, false, nullptr, ETeleportType::TeleportPhysics);
			}
		}
	}
	bIsInFlight = false;
	MovementPhase = EMovementPhase::None;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
