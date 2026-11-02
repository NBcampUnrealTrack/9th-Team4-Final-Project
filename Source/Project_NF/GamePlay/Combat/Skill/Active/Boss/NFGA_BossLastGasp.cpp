#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossLastGasp.h"

#include "Engine/World.h"
#include "GamePlay/Combat/Boss/NF_BossBase.h"
#include "GamePlay/Combat/Boss/NF_BossCombatEffects.h"
#include "GAS/Effects/NF_CommonDamageEffects.h"
#include "GameplayEffect.h"
#include "GAS/Tags/NFTag_BossCombat.h"
#include "EngineUtils.h"
#include "Player/Character/NF_Player.h"
#include "TimerManager.h"

UNFGA_BossLastGasp::UNFGA_BossLastGasp()
{
	ExecutionDuration = TelegraphDuration;
	DamageEffectClass = UNFGE_CommonDamage::StaticClass();
	DamageMagnitudeTag = NFTag_BossCombat::Data_Damage;
	StunEffectClass = UNFGE_BossStun::StaticClass();
}

void UNFGA_BossLastGasp::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ANF_BossBase* Boss = GetBoss();
	if (!Boss || !ActorInfo || !ActorInfo->IsNetAuthority() || !Boss->IsDefeated()
		|| !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	MarkedLocations.Reset();
	for (TActorIterator<ANF_Player> It(GetWorld()); It; ++It)
	{
		ANF_Player* Player = *It;
		if (!IsValid(Player)) continue;
		FVector GroundPoint;
		if (!FindGroundPoint(Player, GroundPoint)) continue;
		MarkedLocations.Add(Player, GroundPoint);
		ShowTelegraph(GroundPoint, ImpactRadius, TelegraphDuration);
	}
	GetWorld()->GetTimerManager().SetTimer(DetonationTimer, this,
		&UNFGA_BossLastGasp::DetonateMarkedZones, FMath::Max(TelegraphDuration, 0.0f), false);
}

void UNFGA_BossLastGasp::DetonateMarkedZones()
{
	if (!IsActive()) return;
	const ANF_BossBase* Boss = GetBoss();
	const FGameplayEffectSpecHandle DamageSpec = MakeBossEffectSpec(DamageEffectClass, DamageMagnitudeTag,
		Boss ? Boss->GetPatternDamage(ENF_BossPatternId::LastGasp, Damage) : Damage);
	const FGameplayEffectSpecHandle StunSpec = MakeBossEffectSpec(StunEffectClass, FGameplayTag(), 0.0f);
	for (const TPair<TWeakObjectPtr<ANF_Player>, FVector>& Entry : MarkedLocations)
	{
		ANF_Player* Player = Entry.Key.Get();
		if (!IsValid(Player) || FVector::DistSquared2D(Player->GetActorLocation(), Entry.Value) > FMath::Square(ImpactRadius))
		{
			continue;
		}
		ApplyEffectToActor(Player, DamageSpec);
		ApplyEffectToActor(Player, StunSpec);
	}
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UNFGA_BossLastGasp::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility, const bool bWasCancelled)
{
	if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(DetonationTimer);
	MarkedLocations.Reset();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	// State-rule/EMP cancellation must still finish the defeated boss's lifecycle.
	if (ANF_BossBase* Boss = GetBoss(); Boss && Boss->IsDefeated() && !Boss->IsActorBeingDestroyed())
	{
		Boss->CompleteDeathSequence();
	}
}
