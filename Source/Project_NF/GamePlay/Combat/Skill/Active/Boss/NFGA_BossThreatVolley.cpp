#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossThreatVolley.h"

#include "GamePlay/Combat/Boss/NF_BossBase.h"
#include "GamePlay/Combat/Boss/NF_BossCombatEffects.h"
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
	DamageEffectClass = UNFGE_BossDamageToPlayer::StaticClass();
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

	const FVector ToTarget = Target->GetActorLocation() - Boss->GetActorLocation();
	const FRotator DesiredRotation(0.0f, ToTarget.Rotation().Yaw, 0.0f);
	Boss->SetActorRotation(FMath::RInterpConstantTo(Boss->GetActorRotation(), DesiredRotation,
		FMath::Max(ShotInterval, 0.01f), FMath::Max(TurnRateDegreesPerSecond, 0.0f)));

	FVector GroundPoint;
	const bool bHasGroundPoint = FindGroundPoint(Target, GroundPoint);
	if (bHasGroundPoint)
	{
		// Resolve the target's ground position when each projectile launches so the
		// first shot uses the player's position after the full telegraph delay.
		if (FiredProjectileCount > 0)
		{
			ShowTelegraph(GroundPoint, ImpactRadius, TelegraphDuration, false,
				ShotTelegraphLifetime, ShotTelegraphMaterial);
		}
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
		GetWorld()->GetTimerManager().SetTimer(RecoveryTimer, this,
			&UNFGA_BossThreatVolley::FinishPattern, FMath::Max(RecoveryDuration, 0.0f), false);
	}
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
	}
	SelectedTarget.Reset();
	FiredProjectileCount = 0;
	ElapsedFireTime = 0.0f;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
