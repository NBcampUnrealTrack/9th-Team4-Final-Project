#include "GamePlay/Combat/Skill/Active/Search/NFGA_Clairvoyance.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/SpectatorPawn.h"
#include "GamePlay/Combat/Components/NF_TargetQueryComponent.h"
#include "GamePlay/Combat/Skill/Execution/NF_SkillExecutionActor.h"
#include "Player/Controller/NF_PlayerController.h"
#include "UI/Minimap/NF_MinimapRevealRegistry.h"
#include "TimerManager.h"

// @owner: 이정준 (구현: Codex) | 2026-10-15
// 목적: 시전자를 따라가는 실시간 플레이어 탐지. 벽 너머도 탐지하며 범위 이탈 시 공개 해제.
UNFGA_Clairvoyance::UNFGA_Clairvoyance()
{
	SkillCategory = ENF_SkillCategory::Search;
	SkillExecutionMode = ENF_SkillExecutionMode::Timed;
	ExecutionDuration = 5.0f;
	BuildCost = 3;
}
bool UNFGA_Clairvoyance::HasValidClairvoyanceSettings() const
{
	return !ExecutionActorClass
		&& NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerOnly && !bRequiresConfirmedTargetData
		&& SkillExecutionMode == ENF_SkillExecutionMode::Timed && FMath::IsFinite(ExecutionDuration) && ExecutionDuration > 0.0f
		&& FMath::IsFinite(CastTime) && CastTime >= 0.0f && FMath::IsFinite(DetectionRadius) && DetectionRadius > 0.0f
		&& FMath::IsFinite(DetectionInterval) && DetectionInterval >= 0.05f;
}
bool UNFGA_Clairvoyance::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	return ActorInfo && Cast<ANF_PlayerController>(ActorInfo->PlayerController.Get()) && HasValidClairvoyanceSettings()
		&& Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}
void UNFGA_Clairvoyance::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!ActorInfo || !ActorInfo->IsNetAuthority() || !GetWorld() || !CanContinueSkillExecution()
		|| !Cast<ANF_PlayerController>(ActorInfo->PlayerController.Get()) || !HasValidClairvoyanceSettings())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return;
	}
	Recipient = Cast<ANF_PlayerController>(ActorInfo->PlayerController.Get());
	Registry = Recipient->GetMinimapRevealRegistry();
	ActivationAvatar = GetAvatarActorFromActorInfo();
	RevealSourceId = FGuid::NewGuid();
	if (CastTime > 0.0f) { GetWorld()->GetTimerManager().SetTimer(CastTimer, this, &ThisClass::StartDetection, CastTime, false); }
	else { StartDetection(); }
}
void UNFGA_Clairvoyance::StartDetection()
{
	if (!IsActive()) { return; }
	if (!Registry.IsValid() || !Recipient.IsValid() || !ActivationAvatar.IsValid()
		|| ActivationAvatar->IsActorBeingDestroyed() || ActivationAvatar.Get() != GetAvatarActorFromActorInfo()
		|| !CanContinueSkillExecution() || !CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true); return;
	}
	if (!IsActive()) { return; }
	EndsAt = GetWorld()->GetTimeSeconds() + ExecutionDuration;
	GetWorld()->GetTimerManager().SetTimer(EndTimer, this, &ThisClass::FinishDetection, ExecutionDuration, false);
	GetWorld()->GetTimerManager().SetTimer(ScanTimer, this, &ThisClass::ScanTargets, DetectionInterval, true);
	ScanTargets();
}
void UNFGA_Clairvoyance::ScanTargets()
{
	if (!IsActive()) { return; }
	AActor* Avatar = ActivationAvatar.Get();
	if (!IsValid(Avatar) || Avatar->IsActorBeingDestroyed() || Avatar != GetAvatarActorFromActorInfo()
		|| !Registry.IsValid() || !Recipient.IsValid() || !CanContinueSkillExecution())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true); return;
	}
	const float Remaining = static_cast<float>(EndsAt - GetWorld()->GetTimeSeconds());
	if (Remaining <= 0.0f) { FinishDetection(); return; }
	FNF_TargetQuerySettings Settings;
	Settings.Shape.Shape = ENF_AreaDamageShape::Sphere; Settings.Shape.Radius = DetectionRadius;
	Settings.Filter = TargetFilter; Settings.Filter.bIncludeSource = false;
	Settings.ObjectChannels = { ECC_Pawn }; Settings.bRequireLineOfSight = false;
	TArray<FNF_TargetQueryResult> Results;
	const FNF_AreaDamageTargetFilter Filter;
	UNF_TargetQueryComponent::Query(GetWorld(), Avatar, Avatar, FTransform(Avatar->GetActorLocation()), Settings, Filter, Results);
	TArray<AActor*> Targets;
	for (const FNF_TargetQueryResult& Result : Results)
	{
		APawn* Pawn = Cast<APawn>(Result.Target);
		if (IsValid(Pawn) && !Pawn->IsA<ASpectatorPawn>() && (Pawn->IsPlayerControlled() || Pawn->GetPlayerState())) { Targets.Add(Pawn); }
	}
	Registry->UpdateRevealSource(RevealSourceId, Targets, ENF_MinimapRevealKind::Clairvoyance, Remaining);
}
void UNFGA_Clairvoyance::FinishDetection()
{
	if (IsActive()) { EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false); }
}
void UNFGA_Clairvoyance::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (!IsEndAbilityValid(Handle, ActorInfo)) { return; }
	if (ScopeLockCount > 0) { Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled); return; }
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(CastTimer); GetWorld()->GetTimerManager().ClearTimer(ScanTimer);
		GetWorld()->GetTimerManager().ClearTimer(EndTimer);
	}
	if (Registry.IsValid()) { Registry->RemoveRevealSource(RevealSourceId); }
	Registry.Reset(); Recipient.Reset(); ActivationAvatar.Reset(); RevealSourceId.Invalidate();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
