#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossCenterBlast.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "GamePlay/Combat/Boss/NF_BossBase.h"
#include "GamePlay/Combat/Boss/NF_BossCombatEffects.h"
#include "GamePlay/Combat/GameplayCue/NF_BossLineTelegraph.h"
#include "GamePlay/Combat/GameplayCue/NF_BossNiagaraBurst.h"
#include "GamePlay/Combat/Skill/Execution/NF_SkillCastVisual.h"
#include "GAS/Effects/NF_CommonDamageEffects.h"
#include "GAS/Tags/NFGameplayCueTags.h"
#include "GAS/Tags/NFTag_BossCombat.h"
#include "GAS/Tags/NFTag_LifeState.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraSystem.h"
#include "Player/Character/NF_Player.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	bool IsLivingFrontStrikeTarget(ANF_Player* Player)
	{
		if (!IsValid(Player)) return false;
		// Players without an initialized ASC (test game modes) still count; they simply cannot receive effects.
		const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Player);
		return !ASC || !ASC->HasMatchingGameplayTag(NFTag_LifeState::State_Life_Dead);
	}
}

UNFGA_BossCenterBlast::UNFGA_BossCenterBlast()
{
	DamageEffectClass = UNFGE_CommonDamage::StaticClass();
	DamageMagnitudeTag = NFTag_BossCombat::Data_Damage;
	StunEffectClass = UNFGE_BossStun::StaticClass();
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> RectTelegraph(
		TEXT("/Game/ProjectNF/Dev/GamePlay/Boss/M_BossLineTelegraph.M_BossLineTelegraph"));
	WarningMaterial = RectTelegraph.Object;
	GetMutableActiveDefaults().ExecutionDuration = GetPlannedDuration();
}

float UNFGA_BossCenterBlast::GetPlannedDuration() const
{
	const int32 Count = FMath::Max(StrikeCount, 1);
	return Count * (FMath::Max(WarningDuration, 0.01f) + FMath::Max(StrikeDelay, 0.0f))
		+ (Count - 1) * FMath::Max(StrikeInterval, 0.0f) + FMath::Max(RecoveryDuration, 0.0f);
}

void UNFGA_BossCenterBlast::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	SetRuntimeExecutionDuration(GetPlannedDuration());
	ANF_BossBase* Boss = GetBoss();
	if (!Boss || !ActorInfo || !ActorInfo->IsNetAuthority() || !WarningMaterial
		|| !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BossFrontStrike] Activation rejected: invalid boss/authority/WarningMaterial or CommitAbility failed. Material=%s"),
			*GetPathNameSafe(WarningMaterial.Get()));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	CurrentStrikeIndex = 0;
	BeginWarning();
}

void UNFGA_BossCenterBlast::BeginWarning()
{
	ANF_BossBase* Boss = GetBoss();
	UWorld* World = GetWorld();
	if (!IsActive() || !IsValid(Boss) || !World || !Boss->HasAuthority()) return;
	if (Boss->IsDefeated())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// Snapshot the area from the boss's facing now. The decal and the damage box use this same transform,
	// so the warned rectangle is exactly the area that gets hit.
	FVector GroundOrigin;
	FindGroundPoint(Boss, GroundOrigin);
	const float Yaw = Boss->GetActorRotation().Yaw;
	const FVector Forward = FRotator(0.0f, Yaw, 0.0f).Vector();
	const float Length = FMath::Max(StrikeLength, 1.0f);
	const FVector Center = GroundOrigin + Forward * (StrikeForwardOffset + Length * 0.5f);
	StrikeAreaTransform = FTransform(FRotator(0.0f, Yaw, 0.0f), Center);
	StrikeExtent = FVector(Length * 0.5f, FMath::Max(StrikeWidth, 1.0f) * 0.5f, FMath::Max(StrikeHalfHeight, 1.0f));

	DestroyWarning();
	const FTransform DecalTransform(FRotator(-90.0f, Yaw, 0.0f), Center);
	Warning = World->SpawnActorDeferred<ANF_BossLineTelegraph>(ANF_BossLineTelegraph::StaticClass(),
		DecalTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Warning)
	{
		// Never damage an area that was not warned.
		UE_LOG(LogTemp, Warning, TEXT("[BossFrontStrike] Warning actor spawn failed; ending pattern."));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}
	Warning->Initialize(WarningMaterial, StrikeExtent);
	Warning->FinishSpawning(DecalTransform);
	UE_LOG(LogTemp, Display, TEXT("[BossFrontStrike] Warning %d/%d Center=%s Yaw=%.1f Extent=%s"),
		CurrentStrikeIndex + 1, FMath::Max(StrikeCount, 1), *Center.ToString(), Yaw, *StrikeExtent.ToString());

	OnCenterBlastTelegraphStarted();
	if (!IsActive()) return;
	World->GetTimerManager().SetTimer(WarningTimer, this, &ThisClass::BeginStrikeWindup,
		FMath::Max(WarningDuration, 0.01f), false);
}

void UNFGA_BossCenterBlast::BeginStrikeWindup()
{
	ANF_BossBase* Boss = GetBoss();
	if (!IsActive() || !IsValid(Boss) || !Boss->HasAuthority()) return;
	if (Boss->IsDefeated())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}
	// The warning stays visible through the wind-up and is removed at the hit.
	PlayStrikeMontage(CurrentStrikeIndex);
	if (!IsActive() || !IsValid(Boss) || Boss->IsDefeated()) return;
	if (StrikeDelay > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(StrikeTimer, this, &ThisClass::Strike, StrikeDelay, false);
	}
	else
	{
		Strike();
	}
}

void UNFGA_BossCenterBlast::Strike()
{
	ANF_BossBase* Boss = GetBoss();
	UWorld* World = GetWorld();
	if (!IsActive() || !IsValid(Boss) || !World || !Boss->HasAuthority()) return;
	if (Boss->IsDefeated())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	DestroyWarning();
	PlayStrikeNiagara(CurrentStrikeIndex, StrikeAreaTransform);
	if (!IsActive() || !IsValid(Boss) || Boss->IsDefeated()) return;
	if (UAbilitySystemComponent* BossASC = Boss->GetAbilitySystemComponent())
	{
		FGameplayCueParameters CueParameters;
		CueParameters.Location = StrikeAreaTransform.GetLocation();
		CueParameters.Normal = StrikeAreaTransform.GetRotation().GetForwardVector();
		CueParameters.RawMagnitude = StrikeExtent.X;
		CueParameters.EffectCauser = Boss;
		BossASC->ExecuteGameplayCue(NFGameplayCueTags::GameplayCue_Boss_CenterBlast_Impact, CueParameters);
	}

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, StrikeAreaTransform.GetLocation(), StrikeAreaTransform.GetRotation(),
		FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeBox(StrikeExtent));
	TSet<ANF_Player*> Victims;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (ANF_Player* Player = Cast<ANF_Player>(Overlap.GetActor()); IsLivingFrontStrikeTarget(Player))
		{
			Victims.Add(Player);
		}
	}

	const FGameplayEffectSpecHandle DamageSpec = MakeBossEffectSpec(DamageEffectClass, DamageMagnitudeTag,
		Boss->GetPatternDamage(ENF_BossPatternId::CenterBlast, Damage));
	const FGameplayEffectSpecHandle StunSpec = MakeBossEffectSpec(StunEffectClass, FGameplayTag(), 0.0f);
	const FVector BossLocation = Boss->GetActorLocation();
	const FVector AreaForward = StrikeAreaTransform.GetRotation().GetForwardVector();
	for (ANF_Player* Player : Victims)
	{
		if (!IsActive()) return;
		ApplyEffectToActor(Player, DamageSpec);
		ApplyEffectToActor(Player, StunSpec);
		FVector Direction = (Player->GetActorLocation() - BossLocation).GetSafeNormal2D();
		if (Direction.IsNearlyZero()) Direction = AreaForward;
		Player->LaunchCharacter(Direction * OutwardLaunchVelocity + FVector(0.0f, 0.0f, UpwardLaunchVelocity), true, true);
	}
	UE_LOG(LogTemp, Display, TEXT("[BossFrontStrike] Strike %d/%d Hit=%d"),
		CurrentStrikeIndex + 1, FMath::Max(StrikeCount, 1), Victims.Num());

	++CurrentStrikeIndex;
	if (CurrentStrikeIndex < FMath::Max(StrikeCount, 1))
	{
		const float Interval = FMath::Max(StrikeInterval, 0.0f);
		if (Interval > 0.0f)
		{
			World->GetTimerManager().SetTimer(NextStrikeTimer, this, &ThisClass::BeginWarning, Interval, false);
		}
		else
		{
			BeginWarning();
		}
	}
	else if (RecoveryDuration > 0.0f)
	{
		World->GetTimerManager().SetTimer(NextStrikeTimer, this, &ThisClass::FinishPattern, RecoveryDuration, false);
	}
	else
	{
		FinishPattern();
	}
}

void UNFGA_BossCenterBlast::PlayStrikeMontage_Implementation(int32 StrikeIndex)
{
	ANF_BossBase* Boss = GetBoss();
	if (!IsActive() || !IsValid(Boss) || !Boss->HasAuthority() || Boss->IsDefeated()) return;
	if (USkeletalMeshComponent* Mesh = Boss->GetBossSkeletalMesh(); IsValid(Mesh) && StrikeMontage)
	{
		ANF_SkillCastVisual::SpawnForCast(Boss, Mesh->GetFName(), StrikeMontage,
			FMath::Max(StrikeMontagePlayRate, 0.01f), StrikeMontageStartSection);
	}
}

void UNFGA_BossCenterBlast::PlayStrikeNiagara_Implementation(int32 StrikeIndex, FTransform AreaTransform)
{
	ANF_BossBase* Boss = GetBoss();
	if (!IsActive() || !IsValid(Boss) || !Boss->HasAuthority() || Boss->IsDefeated() || !ImpactNiagaraSystem) return;
	const FVector Location = AreaTransform.TransformPosition(ImpactNiagaraOffset);
	const FQuat Rotation = AreaTransform.GetRotation() * ImpactNiagaraLocalRotation.Quaternion();
	ANF_BossNiagaraBurst::SpawnBurst(Boss, ImpactNiagaraSystem, Location, Rotation.Rotator(), ImpactNiagaraScale);
}

void UNFGA_BossCenterBlast::DestroyWarning()
{
	if (IsValid(Warning)) Warning->Destroy();
	Warning = nullptr;
}

void UNFGA_BossCenterBlast::FinishPattern()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UNFGA_BossCenterBlast::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility, const bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		FTimerManager& Timers = World->GetTimerManager();
		Timers.ClearTimer(WarningTimer);
		Timers.ClearTimer(StrikeTimer);
		Timers.ClearTimer(NextStrikeTimer);
	}
	DestroyWarning();
	CurrentStrikeIndex = 0;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
