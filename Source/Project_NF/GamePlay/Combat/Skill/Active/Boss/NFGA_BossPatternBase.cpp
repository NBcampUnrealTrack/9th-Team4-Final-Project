#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossPatternBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GamePlay/Combat/Boss/NF_BossBase.h"
#include "GamePlay/Combat/Projectiles/NF_BossAreaProjectile.h"
#include "GameplayEffect.h"
#include "GAS/Tags/NFGameplayCueTags.h"
#include "Materials/MaterialInterface.h"
#include "Player/Character/NF_Player.h"
#include "TimerManager.h"

UNFGA_BossPatternBase::UNFGA_BossPatternBase()
{
	SkillType = ENF_SkillType::Active;
	SkillCategory = ENF_SkillCategory::Attack;
	SkillActivationMethod = ENF_SkillActivationMethod::OnGranted;
	SkillExecutionMode = ENF_SkillExecutionMode::Timed;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

ANF_BossBase* UNFGA_BossPatternBase::GetBoss() const
{
	return Cast<ANF_BossBase>(GetAvatarActorFromActorInfo());
}

FGameplayEffectSpecHandle UNFGA_BossPatternBase::MakeBossEffectSpec(
	const TSubclassOf<UGameplayEffect> EffectClass, const FGameplayTag MagnitudeTag, const float Magnitude) const
{
	if (!EffectClass)
	{
		return FGameplayEffectSpecHandle();
	}
	FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(EffectClass, GetAbilityLevel());
	if (Spec.IsValid() && MagnitudeTag.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(MagnitudeTag, FMath::Max(Magnitude, 0.0f));
	}
	return Spec;
}

void UNFGA_BossPatternBase::ApplyEffectToActor(AActor* Target, const FGameplayEffectSpecHandle& Spec) const
{
	if (!IsValid(Target) || !Spec.IsValid())
	{
		return;
	}
	if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target))
	{
		TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

void UNFGA_BossPatternBase::ApplyDamageAndStun(AActor* Target, const TSubclassOf<UGameplayEffect> DamageEffect,
	const FGameplayTag DamageMagnitudeTag, const float Damage, const TSubclassOf<UGameplayEffect> StunEffect) const
{
	ApplyEffectToActor(Target, MakeBossEffectSpec(DamageEffect, DamageMagnitudeTag, Damage));
	ApplyEffectToActor(Target, MakeBossEffectSpec(StunEffect, FGameplayTag(), 0.0f));
}

void UNFGA_BossPatternBase::ShowTelegraph(const FVector& Location, const float Radius, const float Duration,
	const bool bHeadWarning, const float DecalLifetimeOverride, UMaterialInterface* MaterialOverride) const
{
	if (ANF_BossBase* Boss = GetBoss())
	{
		if (UAbilitySystemComponent* BossASC = Boss->GetAbilitySystemComponent())
		{
			FGameplayCueParameters Parameters;
			Parameters.Location = Location;
			Parameters.RawMagnitude = FMath::Max(Radius, 1.0f);
			Parameters.AbilityLevel = FMath::CeilToInt(FMath::Max(Duration, 0.0f));
			// For manually issued boss telegraph cues, carry an optional per-marker lifetime here.
			Parameters.NormalizedMagnitude = FMath::Max(DecalLifetimeOverride, 0.0f);
			// GameplayCue parameters have no material field; SourceObject carries this optional override.
			Parameters.SourceObject = MaterialOverride;
			Parameters.EffectCauser = Boss;
			BossASC->ExecuteGameplayCue(bHeadWarning ? NFGameplayCueTags::GameplayCue_Boss_TargetWarning
				: NFGameplayCueTags::GameplayCue_Boss_Telegraph, Parameters);
		}
	}
}

bool UNFGA_BossPatternBase::FindGroundPoint(AActor* Target, FVector& OutGroundPoint, const float TraceDistance) const
{
	if (!IsValid(Target) || !Target->GetWorld())
	{
		return false;
	}
	const FVector TargetLocation = Target->GetActorLocation();
	const FVector Start = TargetLocation + FVector(0.0f, 0.0f, 100.0f);
	const FVector End = TargetLocation - FVector(0.0f, 0.0f, FMath::Max(TraceDistance, 1.0f));
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NFBossGroundTrace), false, Target);
	FHitResult Hit;
	if (Target->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		OutGroundPoint = Hit.ImpactPoint;
		return true;
	}
	OutGroundPoint = TargetLocation;
	return true;
}

TArray<ANF_Player*> UNFGA_BossPatternBase::FindPlayersInRange(const FVector& Center, const float Radius) const
{
	TArray<ANF_Player*> Result;
	UWorld* World = GetWorld();
	if (!World)
	{
		return Result;
	}
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NFBossPlayerRange), false, GetBoss());
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, ObjectParams,
		FCollisionShape::MakeSphere(FMath::Max(Radius, 1.0f)), QueryParams);
	TSet<ANF_Player*> Seen;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (ANF_Player* Player = Cast<ANF_Player>(Overlap.GetActor()); IsValid(Player) && !Seen.Contains(Player))
		{
			Seen.Add(Player);
			Result.Add(Player);
		}
	}
	return Result;
}

ANF_BossAreaProjectile* UNFGA_BossPatternBase::SpawnAreaProjectile(
	const TSubclassOf<ANF_BossAreaProjectile> ProjectileClass, const FVector& TargetGroundPoint,
	const float Radius, const float Speed, const FGameplayEffectSpecHandle& DamageSpec,
	const FGameplayTag ImpactCueTag) const
{
	ANF_BossBase* Boss = GetBoss();
	UWorld* World = GetWorld();
	if (!Boss || !World || !Boss->HasAuthority() || !ProjectileClass)
	{
		return nullptr;
	}
	const FVector Origin = Boss->GetActorLocation();
	const FTransform SpawnTransform((TargetGroundPoint - Origin).Rotation(), Origin);
	ANF_BossAreaProjectile* Projectile = World->SpawnActorDeferred<ANF_BossAreaProjectile>(
		ProjectileClass, SpawnTransform, Boss, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Projectile)
	{
		Projectile->InitializeProjectile(TargetGroundPoint, Radius, Speed, DamageSpec, ImpactCueTag);
		Projectile->FinishSpawning(SpawnTransform);
	}
	return Projectile;
}

void UNFGA_BossPatternBase::StartAbilityTimer(FTimerHandle& TimerHandle, const float Delay, TFunction<void()> Callback)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateLambda(MoveTemp(Callback)),
			FMath::Max(Delay, 0.0f), false);
	}
}
