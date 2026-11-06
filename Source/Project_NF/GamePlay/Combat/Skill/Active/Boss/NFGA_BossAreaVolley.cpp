#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossAreaVolley.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "NiagaraSystem.h"
#include "Abilities/GameplayAbility.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameplayCueInterface.h"
#include "GamePlay/Combat/Boss/NF_BossBase.h"
#include "GamePlay/Combat/GameplayCue/NF_BossNiagaraBurst.h"
#include "GamePlay/Combat/Skill/Execution/NF_SkillCastVisual.h"
#include "GAS/Effects/NF_CommonDamageEffects.h"
#include "GamePlay/Combat/Projectiles/NF_BossAreaProjectile.h"
#include "GameplayEffect.h"
#include "GAS/Tags/NFGameplayCueTags.h"
#include "GAS/Tags/NFTag_BossCombat.h"
#include "Project_NF.h"
#include "TimerManager.h"
#include "Player/Character/NF_Player.h"

UNFGA_BossAreaVolley::UNFGA_BossAreaVolley()
{
	GetMutableActiveDefaults().ExecutionDuration = PatternDuration;
	ProjectileClass = ANF_BossAreaProjectile::StaticClass();
	DamageEffectClass = UNFGE_CommonDamage::StaticClass();
	DamageMagnitudeTag = NFTag_BossCombat::Data_Damage;
}

void UNFGA_BossAreaVolley::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!ActorInfo || !ActorInfo->IsNetAuthority() || !ProjectileClass || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ElapsedPatternTime = 0.0f;
	StartedVolleyCount = 0;
	bWarnedMissingDamageConfig = false;
	const int32 PlannedVolleyCount = FMath::Max(MaxVolleysPerPattern, 1);
	const float PlannedEndTime = FMath::Max(VolleyInterval, 0.1f) * (PlannedVolleyCount - 1)
		+ FMath::Max(TelegraphDuration, 0.0f) + FMath::Max(ProjectileLaunchDelay, 0.f) + 0.1f;
	const float LatestEndTime = FMath::Max(PatternDuration, 1.0f)
		+ FMath::Max(TelegraphDuration, 0.f) + FMath::Max(ProjectileLaunchDelay, 0.f) + 0.1f;
	SetRuntimeExecutionDuration(FMath::Min(LatestEndTime, PlannedEndTime));
	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	UWorld* World = IsValid(AvatarActor) ? AvatarActor->GetWorld() : nullptr;
	if (!World)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	World->GetTimerManager().SetTimer(VolleyTimer, this, &UNFGA_BossAreaVolley::StartVolley,
		FMath::Max(VolleyInterval, 0.1f), true);
	World->GetTimerManager().SetTimer(EndTimer, this, &UNFGA_BossAreaVolley::EndPattern,
		LatestEndTime, false);
	StartVolley();
}

void UNFGA_BossAreaVolley::StartVolley()
{
	if (!IsActive())
	{
		return;
	}
	if (StartedVolleyCount >= FMath::Max(MaxVolleysPerPattern, 1)
		|| ElapsedPatternTime >= PatternDuration)
	{
		if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(VolleyTimer);
		return;
	}

	ANF_BossBase* Boss = Cast<ANF_BossBase>(GetAvatarActorFromActorInfo());
	UWorld* World = Boss ? Boss->GetWorld() : nullptr;
	if (!Boss || !World || !Boss->HasAuthority())
	{
		return;
	}


	TArray<AActor*> EligibleTargets;
	for (TActorIterator<ANF_Player> It(World); It; ++It)
	{
		ANF_Player* Player = *It;
		if (IsValid(Player) && FVector::DistSquared(Boss->GetActorLocation(), Player->GetActorLocation()) <= FMath::Square(TargetRange))
		{
			EligibleTargets.Add(Player);
		}
	}

	for (int32 Index = EligibleTargets.Num() - 1; Index > 0; --Index)
	{
		EligibleTargets.Swap(Index, FMath::RandRange(0, Index));
	}

	const int32 TargetCount = FMath::Min(FMath::Clamp(MaxTargetsPerVolley, 1, 5), EligibleTargets.Num());
	UAbilitySystemComponent* BossASC = GetAbilitySystemComponentFromActorInfo();
	TArray<FVector> VolleyTargets;
	for (int32 Index = 0; Index < TargetCount; ++Index)
	{
		FVector GroundPoint;
		AActor* Target = EligibleTargets[Index];
	
		if (!FindGroundPoint(EligibleTargets[Index], GroundPoint))
		{
			continue;
		}
		UE_LOG(LogProject_NF, Warning,
			TEXT("[BossProjectileDiag] TargetSnapshot Target=%s TargetLocation=%s GroundPoint=%s"),
			*GetNameSafe(Target), *Target->GetActorLocation().ToString(), *GroundPoint.ToString());

		if (BossASC)
		{
			// The decal warns where the projectile will land, so it must stay until the impact:
			// telegraph + launch delay + flight time (same distance/speed formula the projectile uses).
			FVector LaunchOrigin = Boss->GetActorLocation();
			if (USkeletalMeshComponent* BossMesh = Boss->GetBossSkeletalMesh();
				IsValid(BossMesh) && BossMesh->DoesSocketExist(ProjectileSpawnSocketName))
			{
				LaunchOrigin = BossMesh->GetSocketLocation(ProjectileSpawnSocketName);
			}
			const float FlightTime = FVector::Distance(LaunchOrigin, GroundPoint) / FMath::Max(ProjectileSpeed, 1.0f);
			const float TimeToImpact = FMath::Max(TelegraphDuration, 0.f) + FMath::Max(ProjectileLaunchDelay, 0.f)
				+ FMath::Max(FlightTime, 0.01f);

			FGameplayCueParameters CueParameters;
			CueParameters.Location = GroundPoint;
			CueParameters.RawMagnitude = ImpactRadius;
			CueParameters.AbilityLevel = FMath::CeilToInt(TimeToImpact);
			// Per-marker lifetime override (see UNFGA_BossPatternBase::ShowTelegraph).
			CueParameters.NormalizedMagnitude = TimeToImpact;
			// GameplayCue parameters have no material field; SourceObject carries the optional override.
			CueParameters.SourceObject = TelegraphMaterial;
			CueParameters.EffectCauser = Boss;
			BossASC->ExecuteGameplayCue(NFGameplayCueTags::GameplayCue_Boss_Telegraph, CueParameters);
		}

		VolleyTargets.Add(GroundPoint);
	}

	if (!VolleyTargets.IsEmpty())
	{
		OnVolleyTelegraphStarted(VolleyTargets.Num());
		if (!IsActive()) return;
		if (TelegraphDuration > 0.f)
		{
			FTimerHandle Timer;
			World->GetTimerManager().SetTimer(Timer,
				FTimerDelegate::CreateUObject(this, &ThisClass::BeginVolleyWindup, VolleyTargets), TelegraphDuration, false);
			TelegraphTimers.Add(Timer);
		}
		else
		{
			BeginVolleyWindup(VolleyTargets);
			if (!IsActive()) return;
		}
	}

	++StartedVolleyCount;
	ElapsedPatternTime += FMath::Max(VolleyInterval, 0.1f);
	if (StartedVolleyCount >= FMath::Max(MaxVolleysPerPattern, 1))
	{
		World->GetTimerManager().ClearTimer(VolleyTimer);
		// Let the final volley finish its warning and spawn its projectiles before ending the ability.
		World->GetTimerManager().SetTimer(EndTimer, this, &UNFGA_BossAreaVolley::EndPattern,
			FMath::Max(TelegraphDuration, 0.0f) + FMath::Max(ProjectileLaunchDelay, 0.f) + 0.1f, false);
	}
}

void UNFGA_BossAreaVolley::FinishTelegraph(FVector_NetQuantize TargetGroundPoint)
{
	// Spawn at the current socket transform after the volley wind-up completes.
	if (!IsActive())
	{
		return;
	}
	ANF_BossBase* Boss = Cast<ANF_BossBase>(GetAvatarActorFromActorInfo());
	UWorld* World = Boss ? Boss->GetWorld() : nullptr;
	if (!Boss || !World || !Boss->HasAuthority())
	{
		return;
	}


	FGameplayEffectSpecHandle DamageSpec;
	if (DamageEffectClass && DamageMagnitudeTag.IsValid())
	{
		const float ConfiguredDamage = Boss->GetPatternDamage(ENF_BossPatternId::AreaVolley, Damage);
		DamageSpec = MakeBossEffectSpec(DamageEffectClass, DamageMagnitudeTag, ConfiguredDamage);
	}
	else if (!bWarnedMissingDamageConfig)
	{
		UE_LOG(LogProject_NF, Warning, TEXT("[BossPattern] Damage GE or SetByCaller tag is not configured; projectile will deal no damage."));
		bWarnedMissingDamageConfig = true;
	}

	const FVector Destination(TargetGroundPoint);
	FVector Origin = Boss->GetActorLocation();
	if (USkeletalMeshComponent* BossMesh = Boss->GetBossSkeletalMesh();
		IsValid(BossMesh) && BossMesh->DoesSocketExist(ProjectileSpawnSocketName))
	{
		Origin = BossMesh->GetSocketLocation(ProjectileSpawnSocketName);
	}
	else
	{
		UE_LOG(LogProject_NF, Warning,
			TEXT("[BossProjectileDiag] Spawn socket '%s' is missing on %s; using actor location %s"),
			*ProjectileSpawnSocketName.ToString(), *GetNameSafe(Boss), *Origin.ToString());
	}
	UE_LOG(LogProject_NF, Warning,
		TEXT("[BossProjectileDiag] SpawnRequest Boss=%s Socket=%s Origin=%s Destination=%s Delta=%s"),
		*GetNameSafe(Boss), *ProjectileSpawnSocketName.ToString(), *Origin.ToString(),
		*Destination.ToString(), *(Destination - Origin).ToString());
	const FTransform SpawnTransform((Destination - Origin).Rotation(), Origin);
	ANF_BossAreaProjectile* Projectile = World->SpawnActorDeferred<ANF_BossAreaProjectile>(
		ProjectileClass, SpawnTransform, Boss, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Projectile)
	{
		return;
	}
	Projectile->InitializeProjectile(Destination, ImpactRadius, ProjectileSpeed, DamageSpec,
		NFGameplayCueTags::GameplayCue_Boss_ProjectileImpact);
	Projectile->FinishSpawning(SpawnTransform);
}

void UNFGA_BossAreaVolley::BeginVolleyWindup(TArray<FVector> Targets)
{
	ANF_BossBase* Boss = GetBoss();
	if (!IsActive() || !IsValid(Boss) || Boss->IsDefeated() || !Boss->HasAuthority()) return;
	USkeletalMeshComponent* Mesh = Boss->GetBossSkeletalMesh();
	if (IsValid(Mesh) && ShotMontage)
	{
		ANF_SkillCastVisual::SpawnForCast(Boss, Mesh->GetFName(), ShotMontage,
			FMath::Max(ShotMontagePlayRate, 0.01f), ShotMontageStartSection);
	}
	if (ProjectileLaunchDelay > 0.f)
	{
		FTimerHandle Timer;
		GetWorld()->GetTimerManager().SetTimer(Timer,
			FTimerDelegate::CreateUObject(this, &ThisClass::LaunchVolley, Targets), ProjectileLaunchDelay, false);
		TelegraphTimers.Add(Timer);
	}
	else LaunchVolley(Targets);
}

void UNFGA_BossAreaVolley::LaunchVolley(TArray<FVector> Targets)
{
	ANF_BossBase* Boss = GetBoss();
	if (!IsActive() || !IsValid(Boss) || Boss->IsDefeated() || !Boss->HasAuthority()) return;
	OnVolleyProjectileLaunch();
	if (!IsActive() || !IsValid(Boss) || Boss->IsDefeated()) return;
	PlayVolleyLaunchCosmetics();
	for (const FVector& Target : Targets)
	{
		if (!IsActive()) return;
		FinishTelegraph(FVector_NetQuantize(Target));
	}
}

void UNFGA_BossAreaVolley::PlayVolleyLaunchCosmetics()
{
	ANF_BossBase* Boss = GetBoss();
	USkeletalMeshComponent* Mesh = Boss ? Boss->GetBossSkeletalMesh() : nullptr;
	if (!Boss || !Boss->HasAuthority() || !IsValid(Mesh)) return;
	if (!LaunchNiagaraSystem) return;
	for (const FName Socket : {LeftMuzzleSocket, RightMuzzleSocket})
	{
		if (Socket.IsNone() || !Mesh->DoesSocketExist(Socket))
		{
			UE_LOG(LogProject_NF, Warning, TEXT("[BossAreaVolley] Launch VFX socket '%s' is missing on %s."),
				*Socket.ToString(), *GetNameSafe(Boss));
			continue;
		}
		const FTransform SocketTransform = Mesh->GetSocketTransform(Socket);
		const FQuat Rotation = SocketTransform.GetRotation() * LaunchEffectLocalRotation.Quaternion();
		ANF_BossNiagaraBurst::SpawnBurst(Boss, LaunchNiagaraSystem,
			SocketTransform.TransformPosition(LaunchEffectLocalOffset), Rotation.Rotator(), LaunchNiagaraScale);
	}
}

bool UNFGA_BossAreaVolley::FindGroundPoint(AActor* Target, FVector& OutGroundPoint) const
{
	if (!IsValid(Target) || !Target->GetWorld())
	{
		return false;
	}
	const FVector TargetLocation = Target->GetActorLocation();
	const FVector TraceStart = TargetLocation + FVector(0.0f, 0.0f, 100.0f);
	const FVector TraceEnd = TargetLocation - FVector(0.0f, 0.0f, FMath::Max(GroundTraceDistance, 1.0f));
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NF_BossGroundPoint), false, Target);
	FHitResult Hit;
	if (Target->GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		OutGroundPoint = Hit.ImpactPoint;
		return true;
	}
	OutGroundPoint = TargetLocation;
	return true;
}

void UNFGA_BossAreaVolley::EndPattern()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UNFGA_BossAreaVolley::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	AActor* AvatarActor = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	UWorld* World = IsValid(AvatarActor) ? AvatarActor->GetWorld() : nullptr;
	if (World)
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		TimerManager.ClearTimer(VolleyTimer);
		TimerManager.ClearTimer(EndTimer);
		for (FTimerHandle& TelegraphTimer : TelegraphTimers)
		{
			TimerManager.ClearTimer(TelegraphTimer);
		}
		TelegraphTimers.Reset();
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
