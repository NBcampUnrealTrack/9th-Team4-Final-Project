#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossLastGasp.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GamePlay/Combat/Boss/NF_BossBase.h"
#include "GamePlay/Combat/Boss/NF_BossCombatEffects.h"
#include "GamePlay/Combat/GameplayCue/NF_BossNiagaraBurst.h"
#include "GAS/Effects/NF_CommonDamageEffects.h"
#include "GameplayEffect.h"
#include "GAS/Tags/NFTag_BossCombat.h"
#include "GAS/Tags/NFTag_LifeState.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraSystem.h"
#include "Player/Character/NF_Player.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

UNFGA_BossLastGasp::UNFGA_BossLastGasp()
{
	GetMutableActiveDefaults().ExecutionDuration = TelegraphDuration;
	DamageEffectClass = UNFGE_CommonDamage::StaticClass();
	DamageMagnitudeTag = NFTag_BossCombat::Data_Damage;
	StunEffectClass = UNFGE_BossStun::StaticClass();
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CircleTelegraph(
		TEXT("/Game/ProjectNF/Dev/GamePlay/Boss/M_BossCircleTelegraph.M_BossCircleTelegraph"));
	TelegraphMaterial = CircleTelegraph.Object;
}

bool UNFGA_BossLastGasp::IsLivingPlayer(const ANF_Player* Player)
{
	if (!IsValid(Player)) return false;
	// Players without an initialized ASC (test game modes) are still marked; they simply cannot take effects.
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<ANF_Player*>(Player));
	return !ASC || !ASC->HasMatchingGameplayTag(NFTag_LifeState::State_Life_Dead);
}

void UNFGA_BossLastGasp::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	SetRuntimeExecutionDuration(TelegraphDuration);
	ANF_BossBase* Boss = GetBoss();
	if (!Boss || !ActorInfo || !ActorInfo->IsNetAuthority() || !Boss->IsDefeated()
		|| !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	MarkedZones.Reset();
	const float Delay = FMath::Max(TelegraphDuration, 0.0f);
	const FVector BossLocation = Boss->GetActorLocation();
	for (TActorIterator<ANF_Player> It(GetWorld()); It; ++It)
	{
		ANF_Player* Player = *It;
		if (!IsLivingPlayer(Player)) continue;
		if (TargetRange > 0.0f && FVector::DistSquared(Player->GetActorLocation(), BossLocation) > FMath::Square(TargetRange)) continue;
		FVector GroundPoint;
		if (!FindGroundPoint(Player, GroundPoint)) continue;
		MarkedZones.Add(GroundPoint);
		// The decal lives exactly until detonation and fills its Progress over the same time.
		ShowTelegraph(GroundPoint, ImpactRadius, Delay, false, Delay, TelegraphMaterial.Get());
		OnLastGaspZoneMarked(GroundPoint, ImpactRadius, Delay);
		if (!IsActive()) return;
	}
	UE_LOG(LogTemp, Display, TEXT("[BossLastGasp] Marked %d zone(s) Radius=%.1f Delay=%.2f Material=%s"),
		MarkedZones.Num(), ImpactRadius, Delay, *GetPathNameSafe(TelegraphMaterial.Get()));
	GetWorld()->GetTimerManager().SetTimer(DetonationTimer, this,
		&UNFGA_BossLastGasp::DetonateMarkedZones, FMath::Max(Delay, 0.01f), false);
}

void UNFGA_BossLastGasp::DetonateMarkedZones()
{
	if (!IsActive()) return;
	ANF_BossBase* Boss = GetBoss();
	const FGameplayEffectSpecHandle DamageSpec = MakeBossEffectSpec(DamageEffectClass, DamageMagnitudeTag,
		Boss ? Boss->GetPatternDamage(ENF_BossPatternId::LastGasp, Damage) : Damage);
	const FGameplayEffectSpecHandle StunSpec = MakeBossEffectSpec(StunEffectClass, FGameplayTag(), 0.0f);
	const TArray<FVector> Zones = MarkedZones;
	TSet<ANF_Player*> Affected;
	for (const FVector& Zone : Zones)
	{
		if (ImpactNiagaraSystem && Boss)
		{
			ANF_BossNiagaraBurst::SpawnBurst(Boss, ImpactNiagaraSystem, Zone + ImpactNiagaraOffset,
				FRotator::ZeroRotator, ImpactNiagaraScale);
		}
		OnLastGaspZoneDetonated(Zone, ImpactRadius);
		if (!IsActive()) return;
		// Every living player standing in the zone is hit, once per detonation even if zones overlap.
		for (TActorIterator<ANF_Player> It(GetWorld()); It; ++It)
		{
			ANF_Player* Player = *It;
			if (!IsLivingPlayer(Player) || Affected.Contains(Player)) continue;
			const FVector PlayerLocation = Player->GetActorLocation();
			if (FVector::DistSquared2D(PlayerLocation, Zone) > FMath::Square(ImpactRadius)
				|| FMath::Abs(PlayerLocation.Z - Zone.Z) > ImpactHalfHeight)
			{
				continue;
			}
			Affected.Add(Player);
			ApplyEffectToActor(Player, DamageSpec);
			ApplyEffectToActor(Player, StunSpec);
		}
	}
	UE_LOG(LogTemp, Display, TEXT("[BossLastGasp] Detonated %d zone(s) AffectedPlayers=%d"), Zones.Num(), Affected.Num());
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UNFGA_BossLastGasp::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility, const bool bWasCancelled)
{
	if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(DetonationTimer);
	MarkedZones.Reset();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	// State-rule/EMP cancellation must still finish the defeated boss's lifecycle.
	if (ANF_BossBase* Boss = GetBoss(); Boss && Boss->IsDefeated() && !Boss->IsActorBeingDestroyed())
	{
		Boss->CompleteDeathSequence();
	}
}
