#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossLineStrike.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GamePlay/Combat/Boss/NF_BossBase.h"
#include "GamePlay/Combat/GameplayCue/NF_BossLineTelegraph.h"
#include "GamePlay/Combat/GameplayCue/NF_BossNiagaraBurst.h"
#include "GamePlay/Combat/Skill/Execution/NF_SkillCastVisual.h"
#include "GAS/Effects/NF_CommonDamageEffects.h"
#include "GAS/Tags/NFTag_BossCombat.h"
#include "GAS/Tags/NFTag_LifeState.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraSystem.h"
#include "Player/Character/NF_Player.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	bool IsLivingPlayer(ANF_Player* Player)
	{
		return IsValid(Player) && Player->GetAbilitySystemComponent()
			&& !Player->GetAbilitySystemComponent()->HasMatchingGameplayTag(NFTag_LifeState::State_Life_Dead);
	}
}

UNFGA_BossLineStrike::UNFGA_BossLineStrike()
{
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(
		TEXT("/Game/ProjectNF/Dev/GamePlay/Boss/M_BossLineTelegraph.M_BossLineTelegraph"));
	WarningMaterial = Material.Object;
}

void UNFGA_BossLineStrike::ActivateAbility(FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ANF_BossBase* Boss = GetBoss();
	UE_LOG(LogTemp, Display, TEXT("[BossLineDiag] Start GA=%s Boss=%s Authority=%d Material=%s Montage=%s Niagara=%s Socket=%s Range=%.1f Warning=%.2f Delay=%.2f"),
		*GetClass()->GetPathName(), *GetNameSafe(Boss), ActorInfo && ActorInfo->IsNetAuthority(), *GetPathNameSafe(WarningMaterial.Get()),
		*GetPathNameSafe(ShotMontage.Get()), *GetPathNameSafe(LaunchNiagaraSystem.Get()), *LaunchEffectSocket.ToString(), TargetRange, WarningDuration, StrikeDelay);
	if (!Boss || !ActorInfo || !ActorInfo->IsNetAuthority() || !WarningMaterial
		|| !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BossLineDiag] Activation rejected: invalid boss/authority/material or CommitAbility failed."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	FVector Origin;
	FindGroundPoint(Boss, Origin);
	TArray<ANF_Player*> Candidates;
	const FVector BossLocation = Boss->GetActorLocation();
	const float RangeSquared = FMath::Square(FMath::Max(TargetRange, 1.f));
	// TEMP: select by actor validity and distance only for boss pattern testing.
	for (TActorIterator<ANF_Player> It(GetWorld()); It; ++It)
	{
		ANF_Player* Player = *It;
		if (IsValid(Player)
			&& FVector::DistSquared(Player->GetActorLocation(), BossLocation) <= RangeSquared)
		{
			Candidates.Add(Player);
		}
	}
	UE_LOG(LogTemp, Display, TEXT("[BossLineDiag] EligibleTargets=%d BossLocation=%s"), Candidates.Num(), *BossLocation.ToString());
	for (int32 Index = Candidates.Num() - 1; Index > 0; --Index)
		Candidates.Swap(Index, FMath::RandRange(0, Index));
	const int32 Count = FMath::Min(Candidates.Num(), FMath::Clamp(MaxTargets, 1, 3));
	for (int32 Index = 0; Index < Count; ++Index)
	{
		FVector Target;
		FindGroundPoint(Candidates[Index], Target);
		const FVector Delta(Target.X - Origin.X, Target.Y - Origin.Y, 0);
		if (Delta.IsNearlyZero()) { UE_LOG(LogTemp, Warning, TEXT("[BossLineDiag] Skipped zero-length line Target=%s"), *GetNameSafe(Candidates[Index])); continue; }
		const FRotator Rotation(0, Delta.Rotation().Yaw, 0);
		const FVector Center = (Origin + Target) * 0.5f;
		const FVector Extent(Delta.Size() * 0.5f, FMath::Max(StrikeWidth, 1.f) * 0.5f,
			FMath::Max(StrikeHalfHeight, 1.f));
		const FTransform DecalTransform(FRotator(-90, Rotation.Yaw, 0), Center);
		auto* Warning = GetWorld()->SpawnActorDeferred<ANF_BossLineTelegraph>(
			ANF_BossLineTelegraph::StaticClass(), DecalTransform, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Warning) { UE_LOG(LogTemp, Warning, TEXT("[BossLineDiag] Warning actor spawn failed.")); continue; } // Never damage an area for which a warning could not be created.
		Warning->Initialize(WarningMaterial, Extent);
		Warning->FinishSpawning(DecalTransform);
		Warnings.Add(Warning);
		StrikeTransforms.Emplace(Rotation, Center);
		StrikeExtents.Add(Extent);
	}
	UE_LOG(LogTemp, Display, TEXT("[BossLineDiag] Selected=%d WarningsCreated=%d"), Count, Warnings.Num());
	if (Warnings.IsEmpty())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}
	GetWorld()->GetTimerManager().SetTimer(WarningTimer, this, &ThisClass::BeginStrikeWindup,
		FMath::Max(WarningDuration, 0.01f), false);
}

void UNFGA_BossLineStrike::BeginStrikeWindup()
{
	ANF_BossBase* Boss = GetBoss();
	if (!IsActive() || !IsValid(Boss) || !Boss->HasAuthority()) return;
	if (Boss->IsDefeated())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}
	UE_LOG(LogTemp, Display, TEXT("[BossLineDiag] Windup: calling PlayStrikeMontage; strike in %.2fs"), StrikeDelay);
	PlayStrikeMontage();
	if (!IsActive() || !IsValid(Boss) || Boss->IsDefeated()) return;
	if (StrikeDelay > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(StrikeTimer, this, &ThisClass::Strike, StrikeDelay, false);
	}
	else Strike();
}

void UNFGA_BossLineStrike::Strike()
{
	ANF_BossBase* Boss = GetBoss();
	if (!IsActive() || !Boss || !Boss->HasAuthority()) return;
	if (Boss->IsDefeated())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}
	UE_LOG(LogTemp, Display, TEXT("[BossLineDiag] Strike: Areas=%d calling PlayStrikeNiagara"), StrikeTransforms.Num());
	PlayStrikeNiagara();
	if (!IsActive() || !IsValid(Boss) || Boss->IsDefeated()) return;
	TSet<ANF_Player*> Victims;
	for (int32 Index = 0; Index < StrikeTransforms.Num(); ++Index)
	{
		TArray<FOverlapResult> Overlaps;
		GetWorld()->OverlapMultiByObjectType(Overlaps, StrikeTransforms[Index].GetLocation(),
			StrikeTransforms[Index].GetRotation(), FCollisionObjectQueryParams(ECC_Pawn),
			FCollisionShape::MakeBox(StrikeExtents[Index]));
		for (const FOverlapResult& Overlap : Overlaps)
			if (ANF_Player* Player = Cast<ANF_Player>(Overlap.GetActor()); IsLivingPlayer(Player))
				Victims.Add(Player);
	}
	UE_LOG(LogTemp, Display, TEXT("[BossLineDiag] HitCandidates=%d"), Victims.Num());
	const FGameplayEffectSpecHandle Spec = MakeBossEffectSpec(UNFGE_CommonDamage::StaticClass(),
		NFTag_BossCombat::Data_Damage, Boss->GetPatternDamage(ENF_BossPatternId::LineStrike, Damage));
	UE_LOG(LogTemp, Display, TEXT("[BossLineDiag] DamageSpecValid=%d"), Spec.IsValid());
	for (ANF_Player* Player : Victims)
	{
		if (!IsActive()) return;
		ApplyEffectToActor(Player, Spec);
	}
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UNFGA_BossLineStrike::PlayStrikeMontage_Implementation()
{
	UE_LOG(LogTemp, Display, TEXT("[BossLineDiag] Native PlayStrikeMontage invoked (BP parent implementation)."));
	ANF_BossBase* Boss = GetBoss();
	if (!IsActive() || !IsValid(Boss) || !Boss->HasAuthority() || Boss->IsDefeated()) return;
	if (USkeletalMeshComponent* Mesh = Boss->GetBossSkeletalMesh(); IsValid(Mesh) && ShotMontage)
	{
		const auto* MontageRelay = ANF_SkillCastVisual::SpawnForCast(Boss, Mesh->GetFName(), ShotMontage,
			FMath::Max(ShotMontagePlayRate, 0.01f), ShotMontageStartSection);
		UE_LOG(LogTemp, Display, TEXT("[BossLineDiag] Montage request Relay=%s Montage=%s"), *GetNameSafe(MontageRelay), *GetPathNameSafe(ShotMontage.Get()));
	}
}

void UNFGA_BossLineStrike::PlayStrikeNiagara_Implementation()
{
	UE_LOG(LogTemp, Display, TEXT("[BossLineDiag] Native PlayStrikeNiagara invoked (BP parent implementation)."));
	ANF_BossBase* Boss = GetBoss();
	if (!IsActive() || !IsValid(Boss) || !Boss->HasAuthority() || Boss->IsDefeated()) return;
	if (LaunchNiagaraSystem)
	{
		USkeletalMeshComponent* Mesh = Boss->GetBossSkeletalMesh();
		if (IsValid(Mesh) && !LaunchEffectSocket.IsNone() && Mesh->DoesSocketExist(LaunchEffectSocket))
		{
			const FTransform SocketTransform = Mesh->GetSocketTransform(LaunchEffectSocket);
			const FQuat Rotation = SocketTransform.GetRotation() * LaunchEffectLocalRotation.Quaternion();
			ANF_BossNiagaraBurst::SpawnBurst(Boss, LaunchNiagaraSystem,
				SocketTransform.TransformPosition(LaunchEffectLocalOffset), Rotation.Rotator(), LaunchNiagaraScale);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[BossLineStrike] Launch VFX socket '%s' is missing on %s."),
				*LaunchEffectSocket.ToString(), *GetNameSafe(Boss));
		}
	}
}

void UNFGA_BossLineStrike::EndAbility(FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	UE_LOG(LogTemp, Display, TEXT("[BossLineDiag] End GA=%s Boss=%s Cancelled=%d Active=%d Warnings=%d Areas=%d"),
		*GetClass()->GetPathName(), *GetNameSafe(GetBoss()), bWasCancelled, IsActive(), Warnings.Num(), StrikeTransforms.Num());
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(WarningTimer);
		GetWorld()->GetTimerManager().ClearTimer(StrikeTimer);
	}
	for (const auto& Warning : Warnings) if (IsValid(Warning)) Warning->Destroy();
	Warnings.Reset();
	StrikeTransforms.Reset();
	StrikeExtents.Reset();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
