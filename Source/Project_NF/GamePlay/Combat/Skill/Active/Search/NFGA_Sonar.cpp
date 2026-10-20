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
#include "UObject/ConstructorHelpers.h"

UNFGA_Sonar::UNFGA_Sonar()
{
	SkillCategory = ENF_SkillCategory::Search; BuildCost = 2;
	SkillExecutionMode = ENF_SkillExecutionMode::Timed; ExecutionDuration = 5.0f;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/ProjectNF/Skills/Sonar/M_PP_Sonar.M_PP_Sonar"));
	if (Material.Succeeded()) { SilhouetteMaterial = Material.Object; }
}

bool UNFGA_Sonar::HasValidSettings() const
{
	return !ExecutionActorClass && !bRequiresConfirmedTargetData && SilhouetteMaterial
		&& SilhouetteMaterial->GetMaterial()->MaterialDomain == MD_PostProcess && SilhouetteStyle.IsValid()
		&& SkillType == ENF_SkillType::Active && SkillExecutionMode == ENF_SkillExecutionMode::Timed
		&& NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerOnly
		&& FMath::IsFinite(DetectionRadius) && DetectionRadius > 0
		&& FMath::IsFinite(CastTime) && CastTime >= 0 && FMath::IsFinite(ExecutionDuration) && ExecutionDuration > 0;
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
	if (CastTime > 0) { GetWorld()->GetTimerManager().SetTimer(CastTimer, this, &ThisClass::EmitPulse, CastTime, false); }
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
	if (!Reveal->Initialize(Recipient, Targets, ExecutionDuration, SilhouetteMaterial, SilhouetteStyle))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true); return;
	}
	EndsAt = GetWorld()->GetTimeSeconds() + ExecutionDuration;
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
