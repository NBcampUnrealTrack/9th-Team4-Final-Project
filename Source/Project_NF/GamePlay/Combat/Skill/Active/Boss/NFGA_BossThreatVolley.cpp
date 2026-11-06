#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossThreatVolley.h"

#include "GamePlay/Combat/Boss/NF_BossBase.h"
#include "GamePlay/Combat/Boss/NF_BossCombatEffects.h"
#include "GAS/Effects/NF_CommonDamageEffects.h"
#include "GamePlay/Combat/Projectiles/NF_BossAreaProjectile.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "GAS/Tags/NFGameplayCueTags.h"
#include "GAS/Tags/NFTag_BossCombat.h"
#include "Player/Character/NF_Player.h"
#include "TimerManager.h"

UNFGA_BossThreatVolley::UNFGA_BossThreatVolley()
{
	ProjectileClass = ANF_BossAreaProjectile::StaticClass();
	DamageEffectClass = UNFGE_CommonDamage::StaticClass();
	DamageMagnitudeTag = NFTag_BossCombat::Data_Damage;
}

void UNFGA_BossThreatVolley::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ANF_BossBase* Boss = GetBoss();
	if (!Boss || !ActorInfo || !ActorInfo->IsNetAuthority() || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UWorld* World = Boss->GetWorld();
	if (!World)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	PatternStartYaw = Boss->GetActorRotation().Yaw;
	bReturningToStartYaw = false;

	TArray<ANF_Player*> EligibleTargets;
	const float TargetRangeSquared = FMath::Square(FMath::Max(TargetRange, 0.0f));
	for (TActorIterator<ANF_Player> It(World); It; ++It)
	{
		ANF_Player* Player = *It;
		if (IsValid(Player)
			&& FVector::DistSquared(Boss->GetActorLocation(), Player->GetActorLocation()) <= TargetRangeSquared)
		{
			EligibleTargets.Add(Player);
		}
	}

	if (EligibleTargets.IsEmpty())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	SelectedTarget = EligibleTargets[FMath::RandRange(0, EligibleTargets.Num() - 1)];
	if (!SelectedTarget.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Keep turning toward the selected target for the whole pattern (charge, telegraph, firing, recovery).
	LastFacingUpdateTime = World->GetTimeSeconds();
	World->GetTimerManager().SetTimer(FacingTimer, this, &UNFGA_BossThreatVolley::UpdateFacing,
		FMath::Max(FacingUpdateInterval, 0.005f), true);

	FVector HeadWarningLocation = SelectedTarget->GetActorLocation() + FVector(0.0f, 0.0f, 180.0f);
	ShowTelegraph(HeadWarningLocation, 120.0f, ChargeDuration, true);
	GetWorld()->GetTimerManager().SetTimer(ChargeTimer, this,
		&UNFGA_BossThreatVolley::BeginAimedVolley, FMath::Max(ChargeDuration, 0.0f), false);
}

void UNFGA_BossThreatVolley::BeginAimedVolley()
{
	ANF_BossBase* Boss = GetBoss();
	ANF_Player* Target = SelectedTarget.Get();
	if (!IsActive() || !Boss || !Target || !Boss->HasAuthority())
	{
		if (IsActive()) EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	FiredProjectileCount = 0;
	ElapsedFireTime = 0.0f;
	GetWorld()->GetTimerManager().SetTimer(TelegraphTimer, this,
		&UNFGA_BossThreatVolley::FireNextProjectile, FMath::Max(TelegraphDuration, 0.0f), false);
}

void UNFGA_BossThreatVolley::FireNextProjectile()
{
	ANF_BossBase* Boss = GetBoss();
	ANF_Player* Target = SelectedTarget.Get();
	if (!IsActive() || !Boss || !Target || !Boss->HasAuthority())
	{
		if (IsActive()) EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	FVector GroundPoint;
	const bool bHasGroundPoint = FindGroundPoint(Target, GroundPoint);
	if (bHasGroundPoint)
	{
		// Resolve the target's ground position when each projectile launches. The decal warns where this shot
		// will land: it lives for the flight time (same Distance/Speed formula as ANF_BossAreaProjectile, which
		// spawns at the boss actor location), so it fills and is removed exactly when damage + impact Niagara play.
		const float FlightTime = FMath::Max(
			FVector::Distance(Boss->GetActorLocation(), GroundPoint) / FMath::Max(ProjectileSpeed, 1.0f), 0.01f);
		ShowTelegraph(GroundPoint, ImpactRadius, FlightTime, false, FlightTime, ShotTelegraphMaterial);
		const FGameplayEffectSpecHandle DamageSpec = MakeBossEffectSpec(DamageEffectClass, DamageMagnitudeTag,
			Boss->GetPatternDamage(ENF_BossPatternId::ThreatVolley, Damage));
		SpawnAreaProjectile(ProjectileClass, GroundPoint, ImpactRadius, ProjectileSpeed, DamageSpec,
			NFGameplayCueTags::GameplayCue_Boss_ThreatVolley_Impact);
	}
	++FiredProjectileCount;
	ElapsedFireTime += FMath::Max(ShotInterval, 0.05f);
	if (ElapsedFireTime < FMath::Max(ProjectileFireDuration, 0.0f))
	{
		GetWorld()->GetTimerManager().SetTimer(ShotTimer, this,
			&UNFGA_BossThreatVolley::FireNextProjectile, FMath::Max(ShotInterval, 0.05f), false);
	}
	else
	{
		// Firing is over: stop tracking the player and turn back to the pre-pattern yaw during recovery.
		// The pattern ends only after the turn back can complete, so the next pattern never starts mid-turn.
		bReturningToStartYaw = true;
		const float RemainingYaw = FMath::Abs(FRotator::NormalizeAxis(PatternStartYaw - Boss->GetActorRotation().Yaw));
		const float ReturnRate = FMath::Max(ReturnTurnRateDegreesPerSecond, 1.0f);
		const float ReturnTime = RemainingYaw / ReturnRate + FMath::Max(FacingUpdateInterval, 0.005f);
		GetWorld()->GetTimerManager().SetTimer(RecoveryTimer, this,
			&UNFGA_BossThreatVolley::FinishPattern, FMath::Max(FMath::Max(RecoveryDuration, 0.0f), ReturnTime), false);
	}
}

void UNFGA_BossThreatVolley::UpdateFacing()
{
	ANF_BossBase* Boss = GetBoss();
	ANF_Player* Target = SelectedTarget.Get();
	UWorld* World = GetWorld();
	if (!IsActive() || !World || !IsValid(Boss) || !Boss->HasAuthority() || (!bReturningToStartYaw && !IsValid(Target)))
	{
		if (World) World->GetTimerManager().ClearTimer(FacingTimer);
		return;
	}
	const double Now = World->GetTimeSeconds();
	const float DeltaSeconds = static_cast<float>(FMath::Max(Now - LastFacingUpdateTime, 0.0));
	LastFacingUpdateTime = Now;
	const FRotator Current = Boss->GetActorRotation();
	float DesiredYaw = PatternStartYaw;
	float TurnRate = ReturnTurnRateDegreesPerSecond;
	if (!bReturningToStartYaw)
	{
		const FVector ToTarget = Target->GetActorLocation() - Boss->GetActorLocation();
		if (ToTarget.SizeSquared2D() <= KINDA_SMALL_NUMBER) return;
		DesiredYaw = ToTarget.Rotation().Yaw;
		TurnRate = TurnRateDegreesPerSecond;
	}
	// Constant-speed interpolation along the shortest arc: smooth, and guaranteed to arrive.
	const FRotator Desired(Current.Pitch, DesiredYaw, Current.Roll);
	Boss->SetActorRotation(FMath::RInterpConstantTo(Current, Desired, DeltaSeconds, FMath::Max(TurnRate, 0.0f)));
}

void UNFGA_BossThreatVolley::FinishPattern()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UNFGA_BossThreatVolley::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility, const bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		FTimerManager& Timers = World->GetTimerManager();
		Timers.ClearTimer(ChargeTimer);
		Timers.ClearTimer(TelegraphTimer);
		Timers.ClearTimer(ShotTimer);
		Timers.ClearTimer(RecoveryTimer);
		Timers.ClearTimer(FacingTimer);
	}
	SelectedTarget.Reset();
	bReturningToStartYaw = false;
	FiredProjectileCount = 0;
	ElapsedFireTime = 0.0f;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
