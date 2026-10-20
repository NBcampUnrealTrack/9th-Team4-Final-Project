#include "GamePlay/Combat/Boss/NF_BossStateTreeTask.h"

#include "GamePlay/Combat/Boss/NF_BossBase.h"
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
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	ANF_BossBase* Boss = GetBossFromStateTreeContext(Context);
	if (!Boss || !InstanceData.AbilityClass || !Boss->ActivateAbilityClass(InstanceData.AbilityClass, InstanceData.bAllowWhenBossDefeated))
	{
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
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
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
	return Boss->IsAbilityClassActive(InstanceData.AbilityClass)
		? EStateTreeRunStatus::Running
		: EStateTreeRunStatus::Succeeded;
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
	return GetBossFromStateTreeContext(Context) ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FNF_BossDormantTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const ANF_BossBase* Boss = GetBossFromStateTreeContext(Context);
	if (!Boss)
	{
		return EStateTreeRunStatus::Failed;
	}
	return Boss->IsAwake() ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}
