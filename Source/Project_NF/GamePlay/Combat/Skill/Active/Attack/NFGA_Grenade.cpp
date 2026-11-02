#include "GamePlay/Combat/Skill/Active/Attack/NFGA_Grenade.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GamePlay/Combat/Projectiles/NF_Grenade.h"
#include "GamePlay/Combat/Skill/Preview/NF_GrenadePreviewActor.h"
#include "GameplayEffect.h"
#include "Project_NF.h"

UNFGA_Grenade::UNFGA_Grenade()
{
	GetMutableSkillDefaults().SkillType = ENF_SkillType::Active;
	GetMutableSkillDefaults().SkillCategory = ENF_SkillCategory::Attack;
	GetMutableSkillDefaults().SkillActivationMethod = ENF_SkillActivationMethod::Input;
	SkillExecutionMode = ENF_SkillExecutionMode::Instant;
	GetMutableSkillDefaults().BuildCost = 2;
	GrenadeClass = ANF_Grenade::StaticClass();
	PreviewActorClass = ANF_GrenadePreviewActor::StaticClass();
	bActivateOnInputRelease = true;
	bRequiresConfirmedTargetData = true;
}

bool UNFGA_Grenade::GetSkillPreviewData_Implementation(AActor* SourceAvatar, FNF_SkillPreviewData& OutData) const
{
	OutData = FNF_SkillPreviewData();
	if (!CalculateThrowParameters(SourceAvatar, OutData.LaunchLocation, OutData.LaunchVelocity))
	{
		return false;
	}
	const ANF_Grenade* Defaults = GrenadeClass->GetDefaultObject<ANF_Grenade>();
	OutData.bHasProjectilePath = true;
	OutData.DisplayRadius = Defaults->GetExplosionRadius();
	OutData.ProjectileRadius = Defaults->GetCollisionRadius();
	OutData.GravityZ = SourceAvatar->GetWorld()->GetGravityZ() * Defaults->GetProjectileGravityScale();
	OutData.FlightTime = Defaults->GetFuseTime();
	return true;
}

bool UNFGA_Grenade::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	return GrenadeClass
		&& Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UNFGA_Grenade::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// C++ 구현이 실행 흐름을 소유한다. Super의 BP Activate 이벤트는 실행하지 않는다.
	const FNF_SkillTargetData* Target = FNF_SkillTargetData::Get(GetConfirmedTargetData());
	if (!ActorInfo || !ActorInfo->IsNetAuthority() || !Target
		|| !ValidateConfirmedTargetData(GetConfirmedTargetData()) || !CanContinueSkillExecution())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	ConfirmedAimDirection = Target->AimDirection;

	if (CastTime > 0.0f)
	{
		UAbilityTask_WaitDelay* Delay = UAbilityTask_WaitDelay::WaitDelay(this, CastTime);
		Delay->OnFinish.AddDynamic(this, &UNFGA_Grenade::ThrowGrenade);
		Delay->ReadyForActivation();
	}
	else
	{
		ThrowGrenade();
	}
}

bool UNFGA_Grenade::CalculateThrowParameters(AActor* Avatar, FVector& OutSpawnLocation, FVector& OutVelocity) const
{
	if (!IsValid(Avatar)) { return false; }
	FVector ViewLocation;
	FRotator ViewRotation;
	Avatar->GetActorEyesViewPoint(ViewLocation, ViewRotation);
	return CalculateThrowParametersForDirection(Avatar, ViewRotation.Vector(), OutSpawnLocation, OutVelocity);
}

bool UNFGA_Grenade::BuildConfirmedTargetData_Implementation(FGameplayAbilityTargetDataHandle& OutTargetData) const
{
	OutTargetData.Clear();
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!IsValid(Avatar)) { return false; }
	FVector Location;
	FRotator Rotation;
	Avatar->GetActorEyesViewPoint(Location, Rotation);
	OutTargetData = MakeSkillTargetData(ENF_SkillTargetKind::Direction, FVector::ZeroVector, Rotation.Vector());
	return true;
}

bool UNFGA_Grenade::ValidateConfirmedTargetData_Implementation(const FGameplayAbilityTargetDataHandle& TargetData) const
{
	const FNF_SkillTargetData* Target = FNF_SkillTargetData::Get(TargetData);
	if (!Super::ValidateConfirmedTargetData_Implementation(TargetData) || !Target
		|| Target->Kind != ENF_SkillTargetKind::Direction) { return false; }
	FVector Spawn;
	FVector Velocity;
	return CalculateThrowParametersForDirection(GetAvatarActorFromActorInfo(), Target->AimDirection, Spawn, Velocity);
}

bool UNFGA_Grenade::CalculateThrowParametersForDirection(AActor* Avatar, const FVector& AimDirection,
	FVector& OutSpawnLocation, FVector& OutVelocity) const
{
	if (!IsValid(Avatar) || !Avatar->GetWorld() || !GrenadeClass)
	{
		return false;
	}

	// 위치/속력/충돌은 서버 값, 방향은 입력을 확정한 순간의 값이다.
	FVector ViewLocation;
	FRotator ViewRotation;
	Avatar->GetActorEyesViewPoint(ViewLocation, ViewRotation);
	ViewRotation = AimDirection.Rotation();
	OutSpawnLocation = ViewLocation + ViewRotation.RotateVector(SpawnOffset);
	if (const ACharacter* Character = Cast<ACharacter>(Avatar))
	{
		if (!ThrowSocketName.IsNone() && Character->GetMesh() && Character->GetMesh()->DoesSocketExist(ThrowSocketName))
		{
			OutSpawnLocation = Character->GetMesh()->GetSocketLocation(ThrowSocketName);
		}
	}

	UWorld* World = Avatar->GetWorld();
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NF_GrenadeThrow), false, Avatar);
	const FVector AimEnd = ViewLocation + ViewRotation.Vector() * FMath::Max(AimDistance, 1.0f);
	FHitResult AimHit;
	World->LineTraceSingleByChannel(AimHit, ViewLocation, AimEnd, ECC_Visibility, QueryParams);
	const FVector AimPoint = AimHit.bBlockingHit ? AimHit.ImpactPoint : AimEnd;
	FRotator ThrowRotation = (AimPoint - OutSpawnLocation).Rotation();
	ThrowRotation.Pitch = FMath::Clamp(FRotator::NormalizeAxis(ThrowRotation.Pitch) + ThrowAngleOffset, -80.0f, 80.0f);
	OutVelocity = ThrowRotation.Vector() * FMath::Max(ThrowSpeed, 1.0f);

	// 벽을 통과한 위치에 생성하거나 벽 내부에서 시작하지 않도록 생성 경로를 검사한다.
	FHitResult SpawnHit;
	const float Radius = GrenadeClass->GetDefaultObject<ANF_Grenade>()->GetCollisionRadius();
	return !World->SweepSingleByChannel(SpawnHit, ViewLocation, OutSpawnLocation, FQuat::Identity,
		ECC_Visibility, FCollisionShape::MakeSphere(Radius), QueryParams);
}

void UNFGA_Grenade::ThrowGrenade()
{
	if (!IsActive())
	{
		return;
	}

	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!ActorInfo || !ActorInfo->IsNetAuthority())
	{
		EndAbility(CurrentSpecHandle, ActorInfo, CurrentActivationInfo, true, true);
		return;
	}
	FVector SpawnLocation;
	FVector Velocity;
	if (!CanContinueSkillExecution() || !CalculateThrowParametersForDirection(Avatar, ConfirmedAimDirection, SpawnLocation, Velocity))
	{
		UE_LOG(LogProject_NF, Warning, TEXT("[Grenade] Throw aborted: invalid actor/class or blocked spawn path."));
		EndAbility(CurrentSpecHandle, ActorInfo, CurrentActivationInfo, true, true);
		return;
	}
	UWorld* World = Avatar->GetWorld();

	// 피해 설정 없이도 투척을 테스트할 수 있다. 설정이 있을 때만 피해 Spec을 만든다.
	FGameplayEffectSpecHandle Spec;
	if (DamageEffectClass && DamageMagnitudeTag.IsValid())
	{
		Spec = MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());
		if (Spec.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(DamageMagnitudeTag, FMath::Max(Damage, 0.0f));
		}
	}

	const FTransform SpawnTransform(Velocity.Rotation(), SpawnLocation);
	ANF_Grenade* Grenade = World->SpawnActorDeferred<ANF_Grenade>(GrenadeClass, SpawnTransform,
		Avatar, Cast<APawn>(Avatar), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Grenade)
	{
		EndAbility(CurrentSpecHandle, ActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// 생성 가능 여부를 확인한 뒤 사용 비용/쿨타임을 적용한다. 준비 중 취소하면 소모하지 않는다.
	if (!CommitAbility(CurrentSpecHandle, ActorInfo, CurrentActivationInfo))
	{
		Grenade->Destroy();
		UE_LOG(LogProject_NF, Warning, TEXT("[Grenade] Throw aborted: CommitAbility failed."));
		EndAbility(CurrentSpecHandle, ActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	Grenade->InitializeGrenade(Velocity, Spec);
	Grenade->FinishSpawning(SpawnTransform);
	UE_LOG(LogProject_NF, Log, TEXT("[Grenade] Spawned %s | Location=%s | Velocity=%s"),
		*GetNameSafe(Grenade), *SpawnLocation.ToString(), *Velocity.ToString());
	EndAbility(CurrentSpecHandle, ActorInfo, CurrentActivationInfo, true, false);
}
