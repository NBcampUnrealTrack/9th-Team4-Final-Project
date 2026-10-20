#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "StateTreeConditionBase.h"
#include "StateTreeTaskBase.h"
#include "NF_BossStateTreeTask.generated.h"

USTRUCT()
struct FNF_BossAbilityTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Ability")
	TSubclassOf<UGameplayAbility> AbilityClass;

	/** Enable this for the terminal last-gasp state, which runs after the boss is defeated. */
	UPROPERTY(EditAnywhere, Category = "Ability")
	bool bAllowWhenBossDefeated = false;
};

/** StateTree task that starts one server-authoritative GAS pattern and waits for it to end. */
USTRUCT(meta = (DisplayName = "NF Activate Boss Gameplay Ability"))
struct PROJECT_NF_API FNF_BossAbilityTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()
	using FInstanceDataType = FNF_BossAbilityTaskInstanceData;

	FNF_BossAbilityTask();
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, float DeltaTime) const override;
};

/** Keeps the encounter idle until the server calls ANF_BossBase::WakeBoss(). */
USTRUCT()
struct FNF_BossDormantTaskInstanceData
{
	GENERATED_BODY()
};

USTRUCT(meta = (DisplayName = "NF Wait For Boss Wake"))
struct PROJECT_NF_API FNF_BossDormantTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()
	using FInstanceDataType = FNF_BossDormantTaskInstanceData;

	FNF_BossDormantTask();
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, float DeltaTime) const override;
};

USTRUCT()
struct FNF_BossDefeatedConditionInstanceData
{
	GENERATED_BODY()
};

/** Transition condition used to move from any combat state into the terminal death state. */
USTRUCT(meta = (DisplayName = "NF Boss Is Defeated"))
struct PROJECT_NF_API FNF_BossDefeatedCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()
	using FInstanceDataType = FNF_BossDefeatedConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
