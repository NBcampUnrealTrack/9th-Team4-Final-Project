#include "GamePlay/Combat/Boss/NF_BossStateTreeTask.h"
#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossLineStrike.h"
#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossGroundDamageZone.h"

#include "GamePlay/Combat/Boss/NF_BossBase.h"
#include "AbilitySystemComponent.h"
#include "StateTreeExecutionContext.h"

static ANF_BossBase* GetBossFromStateTreeContext(FStateTreeExecutionContext& Context)
{
	return Cast<ANF_BossBase>(Context.GetOwner());
}

FNF_BossAbilityTask::FNF_BossAbilityTask()
{
	bShouldCallTick = true;
	bShouldAffectTransitions = true;
}

EStateTreeRunStatus FNF_BossAbilityTask::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.RunningAbilityClass = InstanceData.AbilityClass;
	InstanceData.bStartedLineStrike = false;
	ANF_BossBase* Boss = GetBossFromStateTreeContext(Context);
	if (!Boss)
	{
		UE_LOG(LogTemp, Error, TEXT("[BossStateTreeDiag] Ability task has no ANF_BossBase owner"));
		return EStateTreeRunStatus::Failed;
	}

	if (!InstanceData.AbilityClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[BossStateTreeDiag] Ability task has no configured GA on Boss=%s"), *Boss->GetName());
		return EStateTreeRunStatus::Failed;
	}

	UE_LOG(LogTemp, Display, TEXT("[BossStateTreeDiag] Entering pattern state Boss=%s Ability=%s"),
		*Boss->GetName(), *GetNameSafe(InstanceData.AbilityClass.Get()));
	if (!Boss->ActivateAbilityClass(InstanceData.AbilityClass, InstanceData.bAllowWhenBossDefeated))
	{
		UE_LOG(LogTemp, Error, TEXT("[BossStateTreeDiag] Pattern state failed to activate GA Boss=%s Ability=%s"),
			*Boss->GetName(), *GetNameSafe(InstanceData.AbilityClass.Get()));
		if (Boss && InstanceData.bAllowWhenBossDefeated)
		{
			Boss->CompleteDeathSequence();
		}
		return EStateTreeRunStatus::Failed;
	}
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FNF_BossAbilityTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	ANF_BossBase* Boss = GetBossFromStateTreeContext(Context);
	if (!Boss)
	{
		return EStateTreeRunStatus::Failed;
	}
	if (Boss->IsDefeated() && !InstanceData.bAllowWhenBossDefeated)
	{
		// Leave the state running so its high-priority OnTick transition can enter LastGasp.
		return EStateTreeRunStatus::Running;
	}
	if (Boss->IsAbilityClassActive(InstanceData.RunningAbilityClass))
	{
		return EStateTreeRunStatus::Running;
	}
	if (Boss->WasPatternCancelled(InstanceData.RunningAbilityClass))
	{
		return EStateTreeRunStatus::Failed;
	}
	if (InstanceData.bAppendLineStrikeAfterGroundZone && !InstanceData.bStartedLineStrike
		&& InstanceData.AbilityClass && InstanceData.AbilityClass->IsChildOf(UNFGA_BossGroundDamageZone::StaticClass()))
	{
		InstanceData.bStartedLineStrike = true;
		InstanceData.RunningAbilityClass = Boss->GetLineStrikeAbilityClass();
		return Boss->ActivateAbilityClass(InstanceData.RunningAbilityClass)
			? EStateTreeRunStatus::Running : EStateTreeRunStatus::Failed;
	}
	UE_LOG(LogTemp, Display, TEXT("[BossStateTreeDiag] Pattern GA finished; completing state Boss=%s Ability=%s"),
		*Boss->GetName(), *GetNameSafe(InstanceData.AbilityClass.Get()));
	return EStateTreeRunStatus::Succeeded;
}

void FNF_BossAbilityTask::ExitState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	ANF_BossBase* Boss = GetBossFromStateTreeContext(Context);
	if (!Boss || !InstanceData.RunningAbilityClass
		|| !InstanceData.RunningAbilityClass->IsChildOf(UNFGA_BossLineStrike::StaticClass())) return;
	if (UAbilitySystemComponent* ASC = Boss->GetAbilitySystemComponent())
	{
		if (FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromClass(InstanceData.RunningAbilityClass))
			ASC->CancelAbilityHandle(Spec->Handle);
	}
}

bool FNF_BossDefeatedCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const ANF_BossBase* Boss = GetBossFromStateTreeContext(Context);
	return Boss && Boss->IsDefeated();
}

FNF_BossDormantTask::FNF_BossDormantTask()
{
	bShouldCallTick = true;
	bShouldAffectTransitions = true;
}

EStateTreeRunStatus FNF_BossDormantTask::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	const ANF_BossBase* Boss = GetBossFromStateTreeContext(Context);
	if (!Boss)
	{
		UE_LOG(LogTemp, Error, TEXT("[BossStateTreeDiag] Dormant task has no ANF_BossBase owner"));
		return EStateTreeRunStatus::Failed;
	}
	UE_LOG(LogTemp, Display, TEXT("[BossStateTreeDiag] Entered Dormant Boss=%s Awake=%d"),
		*Boss->GetName(), Boss->IsAwake() ? 1 : 0);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FNF_BossDormantTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const ANF_BossBase* Boss = GetBossFromStateTreeContext(Context);
	if (!Boss)
	{
		return EStateTreeRunStatus::Failed;
	}
	if (Boss->IsAwake())
	{
		UE_LOG(LogTemp, Display, TEXT("[BossStateTreeDiag] Dormant completed; boss is awake Boss=%s"), *Boss->GetName());
		return EStateTreeRunStatus::Succeeded;
	}
	return EStateTreeRunStatus::Running;
}
