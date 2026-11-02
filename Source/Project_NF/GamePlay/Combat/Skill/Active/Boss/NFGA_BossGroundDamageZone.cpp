#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossGroundDamageZone.h"

#include "AbilitySystemComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GamePlay/Combat/Boss/NF_BossBase.h"
#include "GamePlay/Combat/Boss/NF_BossCombatEffects.h"
#include "GAS/Effects/NF_CommonDamageEffects.h"
#include "GameplayEffect.h"
#include "GAS/Tags/NFGameplayCueTags.h"
#include "GAS/Tags/NFTag_BossCombat.h"
#include "Player/Character/NF_Player.h"
#include "TimerManager.h"

UNFGA_BossGroundDamageZone::UNFGA_BossGroundDamageZone()
{
	DamageEffectClass = UNFGE_CommonDamage::StaticClass();
	DamageMagnitudeTag = NFTag_BossCombat::Data_Damage;
}

void UNFGA_BossGroundDamageZone::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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

	ZoneLocations.Reset();
	TArray<ANF_Player*> Candidates;
	const float TargetRangeSquared = FMath::Square(FMath::Max(TargetRange, 0.0f));
	for (TActorIterator<ANF_Player> It(World); It; ++It)
	{
		ANF_Player* Player = *It;
		if (IsValid(Player)
			&& FVector::DistSquared(Boss->GetActorLocation(), Player->GetActorLocation()) <= TargetRangeSquared)
		{
			Candidates.Add(Player);
		}
	}

	for (int32 Index = Candidates.Num() - 1; Index > 0; --Index)
	{
		Candidates.Swap(Index, FMath::RandRange(0, Index));
	}

	const int32 TargetCount = FMath::Min(FMath::Clamp(MaxTargets, 1, 3), Candidates.Num());
	for (int32 Index = 0; Index < TargetCount; ++Index)
	{
		FVector GroundLocation;
		if (FindGroundPoint(Candidates[Index], GroundLocation))
		{
			ZoneLocations.Add(GroundLocation);
			ShowTelegraph(GroundLocation, ZoneRadius, WarningDuration);
		}
	}

	if (ZoneLocations.IsEmpty())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	World->GetTimerManager().SetTimer(WarningTimer, this, &ThisClass::BeginDamageZones,
		FMath::Max(WarningDuration, 0.0f), false);
}

void UNFGA_BossGroundDamageZone::BeginDamageZones()
{
	ANF_BossBase* Boss = GetBoss();
	UWorld* World = GetWorld();
	if (!IsActive() || !Boss || !World || !Boss->HasAuthority())
	{
		if (IsActive())
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		}
		return;
	}

	bZonesActive = true;
	DamageTickCount = 0;
	MaxDamageTicks = FMath::Max(1, FMath::CeilToInt(FMath::Max(ZoneDuration, 0.0f)
		/ FMath::Max(DamageInterval, 0.01f)));
	for (const FVector& Location : ZoneLocations)
	{
		ExecuteZoneCue(NFGameplayCueTags::GameplayCue_Boss_GroundDamageZone_Start, Location);
		OnGroundDamageZoneStarted(Location, ZoneRadius);
	}

	World->GetTimerManager().SetTimer(DamageTickTimer, this, &ThisClass::ApplyDamageTick,
		FMath::Max(DamageInterval, 0.01f), true);
}

void UNFGA_BossGroundDamageZone::ApplyDamageTick()
{
	const ANF_BossBase* Boss = GetBoss();
	if (!IsActive() || !bZonesActive || !Boss)
	{
		return;
	}

	const FGameplayEffectSpecHandle DamageSpec = MakeBossEffectSpec(DamageEffectClass,
		DamageMagnitudeTag, Boss->GetPatternDamage(ENF_BossPatternId::GroundDamageZone, DamagePerTick));
	if (DamageSpec.IsValid())
	{
		for (const FVector& ZoneLocation : ZoneLocations)
		{
			for (ANF_Player* Player : FindPlayersInRange(ZoneLocation, ZoneRadius))
			{
				if (IsValid(Player)
					&& FVector::DistSquared2D(Player->GetActorLocation(), ZoneLocation) <= FMath::Square(ZoneRadius))
				{
					ApplyEffectToActor(Player, DamageSpec);
				}
			}
		}
	}

	++DamageTickCount;
	if (DamageTickCount >= MaxDamageTicks)
	{
		EndDamageZones();
	}
}

void UNFGA_BossGroundDamageZone::EndDamageZones()
{
	if (!bZonesActive)
	{
		return;
	}

	ClearActiveZones();
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UNFGA_BossGroundDamageZone::ExecuteZoneCue(const FGameplayTag CueTag, const FVector& Location) const
{
	if (ANF_BossBase* Boss = GetBoss())
	{
		if (UAbilitySystemComponent* BossASC = Boss->GetAbilitySystemComponent())
		{
			FGameplayCueParameters Parameters;
			Parameters.Location = Location;
			Parameters.RawMagnitude = FMath::Max(ZoneRadius, 1.0f);
			Parameters.EffectCauser = Boss;
			BossASC->ExecuteGameplayCue(CueTag, Parameters);
		}
	}
}

void UNFGA_BossGroundDamageZone::ClearActiveZones()
{
	if (!bZonesActive)
	{
		return;
	}
	bZonesActive = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DamageTickTimer);
	}
	for (const FVector& Location : ZoneLocations)
	{
		ExecuteZoneCue(NFGameplayCueTags::GameplayCue_Boss_GroundDamageZone_End, Location);
		OnGroundDamageZoneEnded(Location, ZoneRadius);
	}
	ZoneLocations.Reset();
}

void UNFGA_BossGroundDamageZone::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility, const bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WarningTimer);
	}
	ClearActiveZones();
	ZoneLocations.Reset();
	DamageTickCount = 0;
	MaxDamageTicks = 0;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
