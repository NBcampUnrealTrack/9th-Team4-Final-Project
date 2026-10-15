#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossAreaVolley.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameplayCueInterface.h"
#include "GamePlay/Combat/Boss/NF_BossBase.h"
#include "GamePlay/Combat/Boss/NF_BossCombatEffects.h"
#include "GamePlay/Combat/Projectiles/NF_BossAreaProjectile.h"
#include "GameplayEffect.h"
#include "GAS/Tags/NFGameplayCueTags.h"
#include "GAS/Tags/NFTag_BossCombat.h"
#include "Project_NF.h"
#include "TimerManager.h"
#include "Player/Character/NF_Player.h"

UNFGA_BossAreaVolley::UNFGA_BossAreaVolley()
{
	SkillType = ENF_SkillType::Active;
	SkillCategory = ENF_SkillCategory::Attack;
	SkillActivationMethod = ENF_SkillActivationMethod::OnGranted;
	SkillExecutionMode = ENF_SkillExecutionMode::Timed;
	ExecutionDuration = PatternDuration;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ProjectileClass = ANF_BossAreaProjectile::StaticClass();
	DamageEffectClass = UNFGE_BossDamageToPlayer::StaticClass();
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
	bWarnedMissingDamageConfig = false;
	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	UWorld* World = IsValid(AvatarActor) ? AvatarActor->GetWorld() : nullptr;
	if (!World)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	StartVolley();
	World->GetTimerManager().SetTimer(VolleyTimer, this, &UNFGA_BossAreaVolley::StartVolley,
		FMath::Max(VolleyInterval, 0.1f), true);
	World->GetTimerManager().SetTimer(EndTimer, this, &UNFGA_BossAreaVolley::EndPattern,
		FMath::Max(PatternDuration, 1.0f), false);
}

void UNFGA_BossAreaVolley::StartVolley()
{
	if (!IsActive())
	{
		return;
	}
	if (ElapsedPatternTime >= PatternDuration)
	{
		return;
	}

	ANF_BossBase* Boss = Cast<ANF_BossBase>(GetAvatarActorFromActorInfo());
	UWorld* World = Boss ? Boss->GetWorld() : nullptr;
	if (!Boss || !World || !Boss->HasAuthority())
	{
		return;
	}
	bLaunchEventTriggeredForVolley = false;

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
	int32 TelegraphCount = 0;
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
			FGameplayCueParameters CueParameters;
			CueParameters.Location = GroundPoint;
			CueParameters.RawMagnitude = ImpactRadius;
			CueParameters.AbilityLevel = FMath::CeilToInt(TelegraphDuration);
			CueParameters.EffectCauser = Boss;
			BossASC->ExecuteGameplayCue(NFGameplayCueTags::GameplayCue_Boss_Telegraph, CueParameters);
		}

		FTimerHandle TelegraphTimer;
		const FTimerDelegate SpawnDelegate = FTimerDelegate::CreateUObject(this,
			&UNFGA_BossAreaVolley::FinishTelegraph, FVector_NetQuantize(GroundPoint));
		World->GetTimerManager().SetTimer(TelegraphTimer, SpawnDelegate,
			FMath::Max(TelegraphDuration, 0.0f), false);
		TelegraphTimers.Add(TelegraphTimer);
		++TelegraphCount;
	}

	if (TelegraphCount > 0)
	{
		OnVolleyTelegraphStarted(TelegraphCount);
	}

	ElapsedPatternTime += FMath::Max(VolleyInterval, 0.1f);
}

void UNFGA_BossAreaVolley::FinishTelegraph(FVector_NetQuantize TargetGroundPoint)
{
	// The 2 second warning delay is tracked per marked location.
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

	if (!bLaunchEventTriggeredForVolley)
	{
		bLaunchEventTriggeredForVolley = true;
		OnVolleyProjectileLaunch();
	}

	FGameplayEffectSpecHandle DamageSpec;
	if (DamageEffectClass && DamageMagnitudeTag.IsValid())
	{
		DamageSpec = MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());
		if (DamageSpec.IsValid())
		{
			DamageSpec.Data->SetSetByCallerMagnitude(DamageMagnitudeTag, FMath::Max(Damage, 0.0f));
		}
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
	Projectile->InitializeProjectile(Destination, ImpactRadius, ProjectileSpeed, DamageSpec);
	Projectile->FinishSpawning(SpawnTransform);
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
