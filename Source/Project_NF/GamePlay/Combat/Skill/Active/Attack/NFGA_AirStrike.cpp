#include "GamePlay/Combat/Skill/Active/Attack/NFGA_AirStrike.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffect.h"
#include "GamePlay/Combat/Skill/Active/Attack/NF_AirStrike.h"
#include "GamePlay/Combat/Components/NF_GroundTargetingComponent.h"
#include "Project_NF.h"

UNFGA_AirStrike::UNFGA_AirStrike()
{
	GetMutableSkillDefaults().SkillType = ENF_SkillType::Active;
	GetMutableSkillDefaults().SkillCategory = ENF_SkillCategory::Attack;
	GetMutableSkillDefaults().SkillActivationMethod = ENF_SkillActivationMethod::Input;
	SkillExecutionMode = ENF_SkillExecutionMode::Instant;
	GetMutableSkillDefaults().BuildCost = 4;
	bActivateOnInputRelease = true;
	bRequiresConfirmedTargetData = true;
	StrikeClass = ANF_AirStrike::StaticClass();
	// 공통 NF_SkillPreviewActor를 부모로 만든 조합 BP를 클래스 기본값에서 지정한다.
	PreviewActorClass = nullptr;
}

float UNFGA_AirStrike::GetStrikeRadius() const
{
	return StrikeClass ? StrikeClass->GetDefaultObject<ANF_AirStrike>()->GetStrikeRadius() : 0.0f;
}

bool UNFGA_AirStrike::CalculateTarget(AActor* Avatar, FVector& OutCenter, FVector& OutFlightDirection) const
{
	OutCenter = FVector::ZeroVector;
	OutFlightDirection = FVector::ZeroVector;
	if (!StrikeClass)
	{
		return false;
	}
	FHitResult Hit;
	if (!UNF_GroundTargetingComponent::CalculateGroundTarget(Avatar, MaxTargetDistance,
		GroundSearchDepth, Hit, OutFlightDirection))
	{
		return false;
	}
	OutCenter = Hit.ImpactPoint;
	return true;
}

bool UNFGA_AirStrike::GetSkillPreviewData_Implementation(AActor* SourceAvatar, FNF_SkillPreviewData& OutData) const
{
	OutData = FNF_SkillPreviewData();
	if (!IsValid(SourceAvatar) || !StrikeClass)
	{
		return false;
	}
	OutData.DisplayRadius = GetStrikeRadius();
	OutData.TargetRange = FMath::Max(MaxTargetDistance, 100.0f);
	OutData.GroundSearchDepth = FMath::Max(GroundSearchDepth, 100.0f);
	return true;
}

bool UNFGA_AirStrike::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	return StrikeClass && StrikeClass->GetDefaultObject<ANF_AirStrike>()->IsStrikeConfigured()
		&& Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UNFGA_AirStrike::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	const FNF_SkillTargetData* Target = FNF_SkillTargetData::Get(GetConfirmedTargetData());
	if (!ActorInfo || !ActorInfo->IsNetAuthority() || !Target || !CanContinueSkillExecution()
		|| !ValidateConfirmedTargetData(GetConfirmedTargetData()))
	{
		UE_LOG(LogProject_NF, Warning, TEXT("[AirStrike] Target selection failed."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	ConfirmedCenter = Target->Location;
	ConfirmedDirection = FVector(Target->AimDirection).GetSafeNormal2D();
	if (CastTime > 0.0f)
	{
		UAbilityTask_WaitDelay* Delay = UAbilityTask_WaitDelay::WaitDelay(this, CastTime);
		Delay->OnFinish.AddDynamic(this, &UNFGA_AirStrike::StartStrike);
		Delay->ReadyForActivation();
	}
	else
	{
		StartStrike();
	}
}

void UNFGA_AirStrike::StartStrike()
{
	if (!IsActive())
	{
		return;
	}
	const FGameplayAbilityActorInfo* Info = GetCurrentActorInfo();
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Info || !Info->IsNetAuthority() || !IsValid(Avatar) || !StrikeClass || !CanContinueSkillExecution()
		|| !ValidateConfirmedTargetData(GetConfirmedTargetData())
		|| FVector::DistSquared2D(Avatar->GetActorLocation(), ConfirmedCenter) > FMath::Square(FMath::Max(MaxTargetDistance, 100.0f)))
	{
		EndAbility(CurrentSpecHandle, Info, CurrentActivationInfo, true, true);
		return;
	}
	FGameplayEffectSpecHandle Spec;
	if (DamageEffectClass && DamageMagnitudeTag.IsValid())
	{
		Spec = MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());
		if (Spec.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(DamageMagnitudeTag, FMath::Max(DamagePerBomb, 0.0f));
		}
	}
	const FTransform Transform(ConfirmedDirection.Rotation(), ConfirmedCenter);
	ANF_AirStrike* Strike = Avatar->GetWorld()->SpawnActorDeferred<ANF_AirStrike>(StrikeClass,
		Transform, Avatar, Cast<APawn>(Avatar), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Strike)
	{
		EndAbility(CurrentSpecHandle, Info, CurrentActivationInfo, true, true);
		return;
	}
	if (!CommitAbility(CurrentSpecHandle, Info, CurrentActivationInfo))
	{
		Strike->Destroy();
		EndAbility(CurrentSpecHandle, Info, CurrentActivationInfo, true, true);
		return;
	}
	Strike->InitializeStrike(ConfirmedCenter, ConfirmedDirection, Spec);
	Strike->FinishSpawning(Transform);
	UE_LOG(LogProject_NF, Log, TEXT("[AirStrike] Started | Center=%s | Radius=%.0f"),
		*ConfirmedCenter.ToString(), GetStrikeRadius());
	EndAbility(CurrentSpecHandle, Info, CurrentActivationInfo, true, false);
}

bool UNFGA_AirStrike::BuildConfirmedTargetData_Implementation(FGameplayAbilityTargetDataHandle& OutTargetData) const
{
	OutTargetData.Clear();
	FVector Center;
	FVector Direction;
	if (!CalculateTarget(GetAvatarActorFromActorInfo(), Center, Direction)) { return false; }
	OutTargetData = MakeSkillTargetData(ENF_SkillTargetKind::GroundPoint, Center, Direction);
	return true;
}

bool UNFGA_AirStrike::ValidateConfirmedTargetData_Implementation(const FGameplayAbilityTargetDataHandle& TargetData) const
{
	const FNF_SkillTargetData* Target = FNF_SkillTargetData::Get(TargetData);
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Super::ValidateConfirmedTargetData_Implementation(TargetData) || !Target
		|| Target->Kind != ENF_SkillTargetKind::GroundPoint || !IsValid(Avatar) || !Avatar->GetWorld()
		|| FMath::Abs(Target->AimDirection.Z) > 0.02f
		|| FVector::DistSquared2D(Avatar->GetActorLocation(), Target->Location) > FMath::Square(FMath::Max(MaxTargetDistance, 100.0f)))
	{
		return false;
	}
	FVector Eyes;
	FRotator Rotation;
	Avatar->GetActorEyesViewPoint(Eyes, Rotation);
	const FVector Center = Target->Location;
	if (FMath::Abs(Eyes.Z - Center.Z) > FMath::Max(GroundSearchDepth, 100.0f) + 1000.0f) { return false; }
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NF_AirStrikeTargetValidation), false, Avatar);
	FHitResult Floor;
	// 네트워크 좌표 양자화 오차를 허용하되 벽/허공의 목표는 거부한다.
	if (!Avatar->GetWorld()->LineTraceSingleByChannel(Floor, Center + FVector::UpVector * 30.0f,
		Center - FVector::UpVector * 30.0f, ECC_Visibility, Params) || Floor.ImpactNormal.Z < 0.3f)
	{
		return false;
	}
	FHitResult Obstacle;
	return !Avatar->GetWorld()->LineTraceSingleByChannel(Obstacle, Eyes, Floor.ImpactPoint + FVector::UpVector * 10.0f,
		ECC_Visibility, Params);
}
