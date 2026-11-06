#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossHomingVolley.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "NiagaraSystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/SkeletalMeshComponent.h"
#include "GamePlay/Combat/Skill/Execution/NF_SkillCastVisual.h"
#include "GamePlay/Combat/GameplayCue/NF_BossNiagaraBurst.h"
#include "GamePlay/Combat/Boss/NF_BossBase.h"
#include "GamePlay/Combat/Boss/NF_BossCombatEffects.h"
#include "GamePlay/Combat/Projectiles/NF_BossHomingProjectile.h"
#include "GAS/Effects/NF_CommonDamageEffects.h"
#include "GAS/Tags/NFTag_BossCombat.h"
#include "GAS/Tags/NFTag_LifeState.h"
#include "Player/Character/NF_Player.h"
#include "TimerManager.h"

UNFGA_BossHomingVolley::UNFGA_BossHomingVolley()
{
	ProjectileClass = ANF_BossHomingProjectile::StaticClass();
	GetMutableActiveDefaults().ExecutionDuration = RoundCount * (3 - 1) * ShotInterval + (RoundCount - 1) * RoundInterval
		+ ANF_BossHomingProjectile::TrackingLifetime;
}

void UNFGA_BossHomingVolley::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ANF_BossBase* Boss = GetBoss();
	UE_LOG(LogTemp, Display, TEXT("[BossHomingDiag] Start GA=%s Boss=%s Authority=%d Projectile=%s Montage=%s Niagara=%s Socket=%s Range=%.1f"),
		*GetClass()->GetPathName(), *GetNameSafe(Boss), ActorInfo && ActorInfo->IsNetAuthority(),
		*GetPathNameSafe(ProjectileClass.Get()), *GetPathNameSafe(ShotMontage.Get()), *GetPathNameSafe(LaunchNiagaraSystem.Get()), *LaunchEffectSocket.ToString(), TargetRange);
	if (!Boss || !ActorInfo || !ActorInfo->IsNetAuthority() || !ProjectileClass ||
		!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BossHomingDiag] Activation rejected: invalid boss/authority/projectile or CommitAbility failed."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	Projectiles.Reset();
	StartedRounds = 0;
	bAllRoundsFinished = false;
	GetWorld()->GetTimerManager().SetTimer(CompletionTimer, this, &ThisClass::CheckProjectilesFinished, 0.05f, true);
	BeginRound();
}

void UNFGA_BossHomingVolley::BeginRound()
{
	ANF_BossBase* Boss = GetBoss();
	if (!IsActive() || !IsValid(Boss)) return;
	++StartedRounds;
	NextTargetIndex = 0;
	RoundTargets.Reset();
	LastShotTime = GetWorld()->GetTimeSeconds();
	// TEMP: permit dead/uninitialized players for boss pattern testing.
	TArray<ANF_Player*> Targets;
	int32 RawTargetCount = 0;
	const float RangeSquared = FMath::Square(FMath::Max(TargetRange, 1.f));
	for (TActorIterator<ANF_Player> It(GetWorld()); It; ++It)
	{
		ANF_Player* Player = *It;
		++RawTargetCount;
		if (IsValid(Player) && FVector::DistSquared(Player->GetActorLocation(), Boss->GetActorLocation()) <= RangeSquared)
			Targets.Add(Player);
	}
	// Nearest eligible players, with stable tie breaking; never select one player twice.
	Targets.Sort([Boss](const ANF_Player& A, const ANF_Player& B)
	{
		const double AD = FVector::DistSquared(A.GetActorLocation(), Boss->GetActorLocation());
		const double BD = FVector::DistSquared(B.GetActorLocation(), Boss->GetActorLocation());
		return AD == BD ? A.GetUniqueID() < B.GetUniqueID() : AD < BD;
	});
	const int32 Count = FMath::Min(Targets.Num(), FMath::Clamp(MaxTargets, 1, 3));
	UE_LOG(LogTemp, Display, TEXT("[BossHomingDiag] Round=%d PlayersFound=%d InRangeTargets=%d Selected=%d BossLocation=%s"),
		StartedRounds, RawTargetCount, Targets.Num(), Count, *Boss->GetActorLocation().ToString());
	for (int32 Index = 0; Index < Count; ++Index) RoundTargets.Add(Targets[Index]);
	FireNextMissile();
}

void UNFGA_BossHomingVolley::FireNextMissile()
{
	if (!IsActive()) return;
	while (NextTargetIndex < RoundTargets.Num())
	{
		ANF_Player* Target = RoundTargets[NextTargetIndex++].Get();
		const bool bFired = FireMissile(Target);
		UE_LOG(LogTemp, Display, TEXT("[BossHomingDiag] Round=%d Shot=%d Target=%s Fired=%d Active=%d"), StartedRounds, NextTargetIndex, *GetNameSafe(Target), bFired, IsActive());
		// A BP launch callback can cancel/end the GA synchronously.
		if (!IsActive()) return;
		if (!bFired) continue;
		LastShotTime = GetWorld()->GetTimeSeconds();
		if (NextTargetIndex < RoundTargets.Num())
		{
			GetWorld()->GetTimerManager().SetTimer(SequenceTimer, this, &ThisClass::FireNextMissile, ShotInterval, false);
			return;
		}
	}
	FinishRound();
}

bool UNFGA_BossHomingVolley::FireMissile(ANF_Player* Target)
{
	ANF_BossBase* Boss = GetBoss();
	// TEMP: match the validity-only test target selection.
	if (!IsValid(Boss) || !IsValid(Target))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BossHomingDiag] Shot skipped: invalid boss or target=%s"), *GetNameSafe(Target));
		return false;
	}
	const auto DamageSpec = MakeBossEffectSpec(UNFGE_CommonDamage::StaticClass(), NFTag_BossCombat::Data_Damage,
		Boss->GetPatternDamage(ENF_BossPatternId::Relocate, Damage));
	auto AirborneSpec = MakeBossEffectSpec(UNFGE_BossAirborne::StaticClass(), FGameplayTag(), 0.f);
	if (!DamageSpec.IsValid() || !AirborneSpec.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BossHomingDiag] Invalid effect specs Damage=%d Airborne=%d"), DamageSpec.IsValid(), AirborneSpec.IsValid());
		return false;
	}
	AirborneSpec.Data->SetDuration(1.f, true);
	{
		// The missile launches from the same point as the launch Niagara (socket + local offset).
		FTransform EffectTransform = Boss->GetActorTransform();
		if (USkeletalMeshComponent* BossMesh = Boss->GetBossSkeletalMesh(); IsValid(BossMesh))
		{
			EffectTransform = !LaunchEffectSocket.IsNone() && BossMesh->DoesSocketExist(LaunchEffectSocket)
				? BossMesh->GetSocketTransform(LaunchEffectSocket) : BossMesh->GetComponentTransform();
		}
		EffectTransform.SetLocation(EffectTransform.TransformPosition(LaunchEffectLocalOffset));
		const FVector LaunchLocation = EffectTransform.GetLocation();
		const FVector ToTarget = Target->GetActorLocation() - LaunchLocation;
		const FTransform Transform(ToTarget.IsNearlyZero() ? Boss->GetActorRotation() : ToTarget.Rotation(), LaunchLocation);
		auto* Projectile = GetWorld()->SpawnActorDeferred<ANF_BossHomingProjectile>(ProjectileClass, Transform,
			Boss, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Projectile) { UE_LOG(LogTemp, Warning, TEXT("[BossHomingDiag] SpawnActorDeferred failed Class=%s"), *GetPathNameSafe(ProjectileClass.Get())); return false; }
		Projectile->InitializeProjectile(Target, ProjectileSpeed, DetectionRadius, ExplosionRadius,
			AirborneLaunchVelocity, DamageSpec, AirborneSpec);
		Projectiles.Add(Projectile);
		Projectile->FinishSpawning(Transform);
		UE_LOG(LogTemp, Display, TEXT("[BossHomingDiag] Spawn Projectile=%s Location=%s Target=%s Speed=%.1f Valid=%d"), *GetNameSafe(Projectile), *Transform.GetLocation().ToString(), *GetNameSafe(Target), ProjectileSpeed, IsValid(Projectile));
		if (!IsValid(Projectile) || Projectile->IsActorBeingDestroyed() || !IsActive()) return false;
		if (ShotMontage && IsValid(Boss->GetBossSkeletalMesh()))
		{
			const auto* MontageRelay = ANF_SkillCastVisual::SpawnForCast(Boss, Boss->GetBossSkeletalMesh()->GetFName(), ShotMontage,
				FMath::Max(ShotMontagePlayRate, 0.01f), ShotMontageStartSection);
		UE_LOG(LogTemp, Display, TEXT("[BossHomingDiag] Montage request Relay=%s Montage=%s"), *GetNameSafe(MontageRelay), *GetPathNameSafe(ShotMontage.Get()));
		}
		EffectTransform.SetScale3D(LaunchNiagaraScale);
		ANF_BossNiagaraBurst::SpawnBurst(Boss, LaunchNiagaraSystem, EffectTransform.GetLocation(),
			EffectTransform.Rotator(), LaunchNiagaraScale);
		OnHomingMissileLaunched(Boss, Projectile, EffectTransform);
	}
	return true;
}

void UNFGA_BossHomingVolley::FinishRound()
{
	if (!IsActive()) return;
	if (StartedRounds < RoundCount)
	{
		// Invalid/departed targets must not extend the wait beyond the last actual shot.
		const float Delay = FMath::Max(static_cast<float>(LastShotTime + RoundInterval - GetWorld()->GetTimeSeconds()), 0.01f);
		UE_LOG(LogTemp, Display, TEXT("[BossHomingDiag] Round=%d complete; next round in %.2fs TrackedProjectiles=%d"), StartedRounds, Delay, Projectiles.Num());
		GetWorld()->GetTimerManager().SetTimer(SequenceTimer, this, &ThisClass::BeginRound, Delay, false);
	}
	else
	{
		bAllRoundsFinished = true;
		CheckProjectilesFinished();
	}
}

void UNFGA_BossHomingVolley::CheckProjectilesFinished()
{
	Projectiles.RemoveAll([](const auto& Projectile) { return !Projectile.IsValid() || Projectile->IsActorBeingDestroyed(); });
	if (bAllRoundsFinished && Projectiles.IsEmpty() && IsActive()) EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UNFGA_BossHomingVolley::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	UE_LOG(LogTemp, Display, TEXT("[BossHomingDiag] End GA=%s Boss=%s Cancelled=%d Active=%d Rounds=%d AllRoundsFinished=%d TrackedProjectiles=%d"),
		*GetClass()->GetPathName(), *GetNameSafe(GetBoss()), bWasCancelled, IsActive(), StartedRounds, bAllRoundsFinished, Projectiles.Num());
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(CompletionTimer);
		GetWorld()->GetTimerManager().ClearTimer(SequenceTimer);
	}
	RoundTargets.Reset();
	for (auto& Projectile : Projectiles)
	{
		if (Projectile.IsValid()) Projectile->Destroy();
	}
	Projectiles.Reset();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
