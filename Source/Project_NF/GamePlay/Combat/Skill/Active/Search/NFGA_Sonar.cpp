#include "GamePlay/Combat/Skill/Active/Search/NFGA_Sonar.h"
#include "GamePlay/Combat/Components/NF_TargetQueryComponent.h"
#include "GamePlay/Combat/Skill/Execution/NF_SkillExecutionActor.h"
#include "GamePlay/Combat/Skill/Execution/NF_SonarReveal.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "TimerManager.h"

UNFGA_Sonar::UNFGA_Sonar()
{
	GetMutableSkillDefaults().SkillCategory = ENF_SkillCategory::Search; GetMutableSkillDefaults().BuildCost = 2;
	GetMutableActiveDefaults().ExecutionMode = ENF_SkillExecutionMode::Timed; GetMutableActiveDefaults().ExecutionDuration = 5.0f;
}

bool UNFGA_Sonar::HasValidSettings() const
{
	return !GetActiveSettings().ExecutionActorClass && !GetActiveSettings().bRequiresConfirmedTargetData && SilhouetteMaterial
		&& SilhouetteMaterial->GetMaterial()->MaterialDomain == MD_PostProcess && SilhouetteStyle.IsValid()
		&& GetSkillType() == ENF_SkillType::Active && GetSkillExecutionMode() == ENF_SkillExecutionMode::Timed
		&& NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerOnly
		&& FMath::IsFinite(DetectionRadius) && DetectionRadius > 0
		&& FMath::IsFinite(GetCastTime()) && GetCastTime() >= 0 && FMath::IsFinite(GetExecutionDuration()) && GetExecutionDuration() > 0;
}

bool UNFGA_Sonar::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	return ActorInfo && ActorInfo->PlayerController.IsValid() && HasValidSettings()
		&& Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UNFGA_Sonar::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!ActorInfo || !ActorInfo->IsNetAuthority() || !ActorInfo->PlayerController.IsValid() || !HasValidSettings() || !CanContinueSkillExecution())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return;
	}
	OriginalAvatar = GetAvatarActorFromActorInfo();
	if (GetCastTime() > 0) { GetWorld()->GetTimerManager().SetTimer(CastTimer, this, &ThisClass::EmitPulse, GetCastTime(), false); }
	else { EmitPulse(); }
}

void UNFGA_Sonar::EmitPulse()
{
	if (!IsActive()) { return; }
	AActor* Avatar = OriginalAvatar.Get();
	APlayerController* Recipient = CurrentActorInfo ? CurrentActorInfo->PlayerController.Get() : nullptr;
	if (!IsValid(Avatar) || Avatar->IsActorBeingDestroyed() || Avatar != GetAvatarActorFromActorInfo() || !IsValid(Recipient)
		|| !CanContinueSkillExecution() || !HasValidSettings())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true); return;
	}
	FNF_TargetQuerySettings Settings;
	Settings.Shape.Shape = ENF_AreaDamageShape::Sphere; Settings.Shape.Radius = DetectionRadius;
	Settings.Filter = TargetFilter; Settings.Filter.bIncludeSource = false;
	Settings.ObjectChannels = { ECC_Pawn }; Settings.bRequireLineOfSight = false;
	TArray<FNF_TargetQueryResult> Results;
	UNF_TargetQueryComponent::Query(GetWorld(), Avatar, Avatar, FTransform(Avatar->GetActorLocation()), Settings, FNF_AreaDamageTargetFilter(), Results);
	TArray<TObjectPtr<ACharacter>> Targets;
	for (const auto& Result : Results)
	{
		if (ACharacter* Character = Cast<ACharacter>(Result.Target)) { Targets.AddUnique(Character); }
	}
	FActorSpawnParameters Params; Params.Owner = Recipient;
	Reveal = GetWorld()->SpawnActor<ANF_SonarReveal>(Params);
	if (!IsValid(Reveal) || !CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true); return;
	}
	if (!IsActive()) { return; }
	if (!Reveal->Initialize(Recipient, Targets, GetExecutionDuration(), SilhouetteMaterial, SilhouetteStyle))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true); return;
	}
	EndsAt = GetWorld()->GetTimeSeconds() + GetExecutionDuration();
	GetWorld()->GetTimerManager().SetTimer(LifetimeTimer, this, &ThisClass::CheckLifetime, 0.05f, true);
}

void UNFGA_Sonar::CheckLifetime()
{
	if (!IsActive()) { return; }
	const bool bCancelled = !OriginalAvatar.IsValid() || OriginalAvatar->IsActorBeingDestroyed()
		|| OriginalAvatar.Get() != GetAvatarActorFromActorInfo() || !CanContinueSkillExecution() || !IsValid(Reveal);
	if (bCancelled || GetWorld()->GetTimeSeconds() >= EndsAt)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bCancelled);
	}
}

void UNFGA_Sonar::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (!IsEndAbilityValid(Handle, ActorInfo)) { return; }
	if (ScopeLockCount > 0) { Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled); return; }
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(CastTimer); GetWorld()->GetTimerManager().ClearTimer(LifetimeTimer);
	}
	if (IsValid(Reveal)) { Reveal->StopReveal(); Reveal->SetLifeSpan(0.25f); }
	Reveal = nullptr; OriginalAvatar.Reset();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
