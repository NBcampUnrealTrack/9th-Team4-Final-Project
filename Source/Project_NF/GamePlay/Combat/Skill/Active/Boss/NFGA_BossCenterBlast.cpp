#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossCenterBlast.h"

#include "Engine/World.h"
#include "GamePlay/Combat/Boss/NF_BossBase.h"
#include "GamePlay/Combat/Boss/NF_BossCombatEffects.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GAS/Tags/NFGameplayCueTags.h"
#include "GAS/Tags/NFTag_BossCombat.h"
#include "Player/Character/NF_Player.h"
#include "TimerManager.h"

UNFGA_BossCenterBlast::UNFGA_BossCenterBlast()
{
	ExecutionDuration = TelegraphDuration;
	DamageEffectClass = UNFGE_BossDamageToPlayer::StaticClass();
	DamageMagnitudeTag = NFTag_BossCombat::Data_Damage;
	StunEffectClass = UNFGE_BossStun::StaticClass();
}

void UNFGA_BossCenterBlast::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ANF_BossBase* Boss = GetBoss();
	if (!Boss || !ActorInfo || !ActorInfo->IsNetAuthority() || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	ShowTelegraph(Boss->GetActorLocation(), BlastRadius, TelegraphDuration);
	OnCenterBlastTelegraphStarted();
	GetWorld()->GetTimerManager().SetTimer(DetonationTimer, this, &UNFGA_BossCenterBlast::Detonate,
		FMath::Max(TelegraphDuration, 0.0f), false);
}

void UNFGA_BossCenterBlast::Detonate()
{
	ANF_BossBase* Boss = GetBoss();
	if (!IsActive() || !Boss || !Boss->HasAuthority())
	{
		if (IsActive()) EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	const FVector Center = Boss->GetActorLocation();
	if (UAbilitySystemComponent* BossASC = Boss->GetAbilitySystemComponent())
	{
		FGameplayCueParameters CueParameters;
		CueParameters.Location = Center;
		CueParameters.RawMagnitude = BlastRadius;
		CueParameters.EffectCauser = Boss;
		BossASC->ExecuteGameplayCue(NFGameplayCueTags::GameplayCue_Boss_CenterBlast_Impact, CueParameters);
	}

	const FGameplayEffectSpecHandle DamageSpec = MakeBossEffectSpec(DamageEffectClass, DamageMagnitudeTag, Damage);
	const FGameplayEffectSpecHandle StunSpec = MakeBossEffectSpec(StunEffectClass, FGameplayTag(), 0.0f);
	for (ANF_Player* Player : FindPlayersInRange(Center, BlastRadius))
	{
		ApplyEffectToActor(Player, DamageSpec);
		ApplyEffectToActor(Player, StunSpec);
		const FVector Direction = (Player->GetActorLocation() - Center).GetSafeNormal2D();
		Player->LaunchCharacter(Direction * OutwardLaunchVelocity + FVector(0.0f, 0.0f, UpwardLaunchVelocity), true, true);
	}
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UNFGA_BossCenterBlast::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility, const bool bWasCancelled)
{
	if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(DetonationTimer);
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
