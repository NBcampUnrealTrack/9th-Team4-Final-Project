#include "GamePlay/Combat/Skill/Active/Utility/NFGA_EMP.h"
#include "GamePlay/Combat/Components/NF_TargetQueryComponent.h"
#include "GamePlay/Combat/Components/NF_ApplyEffectComponent.h"
#include "GamePlay/Combat/Skill/Execution/NF_SkillExecutionActor.h"
#include "GAS/Effects/NFGE_EMP.h"
#include "GAS/Tags/NFTag_Skill.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

UNFGA_EMP::UNFGA_EMP()
{
	GetMutableSkillDefaults().SkillCategory = ENF_SkillCategory::Utility; GetMutableSkillDefaults().BuildCost = 3;
	GetMutableActiveDefaults().ExecutionMode = ENF_SkillExecutionMode::Instant;
	EMPEffectClass = UNFGE_EMP::StaticClass();
}

bool UNFGA_EMP::HasValidSettings() const
{
	return !GetActiveSettings().bRequiresConfirmedTargetData && EMPEffectClass
		&& EMPEffectClass->GetDefaultObject<UNFGE_EMP>()->DurationPolicy == EGameplayEffectDurationType::HasDuration
		&& EMPEffectClass->GetDefaultObject<UNFGE_EMP>()->GetGrantedTags().HasTag(NFTag_Skill::State_Debuff_EMP)
		&& GetSkillType() == ENF_SkillType::Active && GetSkillExecutionMode() == ENF_SkillExecutionMode::Instant
		&& NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerOnly
		&& FMath::IsFinite(Radius) && Radius > 0 && FMath::IsFinite(SealDuration) && SealDuration >= 0.05f
		&& FMath::IsFinite(GetCastTime()) && GetCastTime() >= 0
		&& FMath::IsFinite(CosmeticDuration) && CosmeticDuration >= 0.1f;
}

bool UNFGA_EMP::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	return HasValidSettings() && Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UNFGA_EMP::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!ActorInfo || !ActorInfo->IsNetAuthority() || !HasValidSettings() || !CanContinueSkillExecution())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return;
	}
	OriginalAvatar = GetAvatarActorFromActorInfo();
	if (GetCastTime() > 0) { GetWorld()->GetTimerManager().SetTimer(CastTimer, this, &ThisClass::EmitPulse, GetCastTime(), false); }
	else { EmitPulse(); }
}

void UNFGA_EMP::EmitPulse()
{
	if (!IsActive()) { return; }
	AActor* Avatar = OriginalAvatar.Get();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(Avatar) || Avatar->IsActorBeingDestroyed() || Avatar != GetAvatarActorFromActorInfo() || !ASC
		|| !HasValidSettings() || !CanContinueSkillExecution())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true); return;
	}
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EMPEffectClass, GetAbilityLevel(), ASC->MakeEffectContext());
	if (!Spec.IsValid() || !CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true); return;
	}
	if (!IsActive()) { return; }
	Spec.Data->SetDuration(SealDuration, true);
	FNF_TargetQuerySettings Settings;
	Settings.Shape.Shape = ENF_AreaDamageShape::Sphere; Settings.Shape.Radius = Radius;
	Settings.Filter = TargetFilter; Settings.Filter.bIncludeSource = false; Settings.Filter.bRequireAbilitySystem = true;
	Settings.bDeduplicateAbilitySystems = true;
	Settings.ObjectChannels = { ECC_Pawn }; Settings.bRequireLineOfSight = bRequireLineOfSight;
	TArray<FNF_TargetQueryResult> Results;
	UNF_TargetQueryComponent::Query(GetWorld(), Avatar, Avatar, FTransform(Avatar->GetActorLocation()), Settings, FNF_AreaDamageTargetFilter(), Results);
	const FVector Origin = Avatar->GetActorLocation();
	// BP 실행 액터는 연출만 담당한다. EMP 효과 적용과 GA 수명은 기존 경로를 유지한다.
	if (GetActiveSettings().ExecutionActorClass || bShowRadius)
	{
		TSubclassOf<ANF_SkillExecutionActor> VisualClass = GetActiveSettings().ExecutionActorClass;
		if (!VisualClass) { VisualClass = ANF_SkillExecutionActor::StaticClass(); }
		FActorSpawnParameters Params;
		Params.Owner = Avatar;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ANF_SkillExecutionActor* Visual = GetWorld()->SpawnActor<ANF_SkillExecutionActor>(VisualClass, Origin, FRotator::ZeroRotator, Params);
		if (IsValid(Visual))
		{
			if (!Visual->StartCosmeticExecution(Avatar, CosmeticDuration, Radius, bShowRadius)) { Visual->Destroy(); }
		}
		else { UE_LOG(LogTemp, Warning, TEXT("[EMP] Failed to spawn cosmetic execution actor; EMP gameplay remains enabled.")); }
	}
	for (const auto& Result : Results)
	{
		if (!IsActive()) { break; }
		UNF_ApplyEffectComponent::SubmitSpec(Avatar, Result.Target, Spec, Origin, Result.Hit);
	}
	if (IsActive()) { EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false); }
}

void UNFGA_EMP::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (!IsEndAbilityValid(Handle, ActorInfo)) { return; }
	if (ScopeLockCount > 0) { Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled); return; }
	if (GetWorld()) { GetWorld()->GetTimerManager().ClearTimer(CastTimer); }
	OriginalAvatar.Reset();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
