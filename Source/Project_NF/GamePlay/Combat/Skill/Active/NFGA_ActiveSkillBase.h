#pragma once

#include "GamePlay/Combat/Skill/NFGA_SkillBase.h"
#include "NFGA_ActiveSkillBase.generated.h"

class UAnimMontage;
class ANF_SkillPreviewActor;
class ANF_SkillExecutionActor;

UCLASS(Blueprintable)
class PROJECT_NF_API UNFGA_ActiveSkillBase : public UNFGA_SkillBase
{
	GENERATED_BODY()
public:
	UNFGA_ActiveSkillBase();
	virtual bool BeginLocalInput() override;
	virtual bool ConfirmLocalInput(FGameplayAbilityTargetDataHandle& OutTargetData) override;
	virtual void CancelLocalInput() override;
	virtual bool IsLocalInputHolding() const override { return bLocalInputHolding; }
	virtual bool ShouldActivateOnInputRelease() const override { return bActivateOnInputRelease; }
	virtual bool ShouldEndOnInputRelease() const override;
	virtual void ReleaseSkillInput() override;
	virtual bool PrepareConfirmedTargetData(const FGameplayAbilityTargetDataHandle& TargetData) override;
	virtual void ClearConfirmedTargetData() override;
	virtual FGameplayAbilityTargetDataHandle GetConfirmedTargetData() const override { return ConfirmedTargetData; }
	virtual ENF_SkillExecutionMode GetSkillExecutionMode() const override { return SkillExecutionMode; }
	virtual float GetCastTime() const override { return CastTime; }
	virtual float GetExecutionDuration() const override { return ExecutionDuration; }
	virtual bool CanContinueSkillExecution() const override;
	virtual bool IsAffectedByEMP() const override;
	virtual bool IsBlockedByEMP() const override;
	virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	UFUNCTION(BlueprintCallable, Category = "NF|Skill|Preview")
	bool BeginLocalPreview();
	UFUNCTION(BlueprintCallable, Category = "NF|Skill|Preview")
	void EndLocalPreview();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NF|Skill|Preview")
	bool GetSkillPreviewData(AActor* SourceAvatar, FNF_SkillPreviewData& OutData) const;
	virtual bool GetSkillPreviewData_Implementation(AActor* SourceAvatar, FNF_SkillPreviewData& OutData) const;
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NF|Skill|Targeting")
	bool BuildConfirmedTargetData(FGameplayAbilityTargetDataHandle& OutTargetData) const;
	virtual bool BuildConfirmedTargetData_Implementation(FGameplayAbilityTargetDataHandle& OutTargetData) const;
	UFUNCTION(BlueprintNativeEvent, Category = "NF|Skill|Targeting")
	bool ValidateConfirmedTargetData(const FGameplayAbilityTargetDataHandle& TargetData) const;
	virtual bool ValidateConfirmedTargetData_Implementation(const FGameplayAbilityTargetDataHandle& TargetData) const;
protected:
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual void CommitExecute(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo) override;
	virtual void CollectStateRestrictionTags(FGameplayTagContainer& Tags) const override;
	virtual void HandleStateRestrictionChanged(FGameplayTag Tag, int32 Count) override;
	UFUNCTION(BlueprintImplementableEvent, Category = "NF|Skill|Input")
	void LocalInputCanceled();
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Animation", meta = (ToolTip = "사용 확정 시 스킬 효과와 동시에 재생할 연출 전용 몽타주입니다. None이면 재생하지 않습니다. 공격 판정 Notify가 없는 몽타주를 지정하세요. 루트 모션 몽타주는 이동에 영향을 주므로 지원하지 않습니다."))
	TObjectPtr<UAnimMontage> CastMontage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Animation", meta = (ClampMin = "0.01", ToolTip = "시전 모션의 재생 속도입니다. 스킬 효과 발생 시점이나 쿨타임은 변경하지 않습니다."))
	float CastMontagePlayRate = 1.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Animation", meta = (ToolTip = "재생을 시작할 몽타주 섹션입니다. None이면 처음부터 재생합니다."))
	FName CastMontageStartSection = NAME_None;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|EMP")
	bool bAffectedByEMP = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|EMP", meta = (EditCondition = "bAffectedByEMP"))
	bool bCancelOnEMP = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Execution")
	TSubclassOf<ANF_SkillExecutionActor> ExecutionActorClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Preview")
	FNF_SkillPreviewData DefaultPreviewData;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Input")
	FGameplayTagContainer InputCancelTags;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Targeting")
	bool bRequiresConfirmedTargetData = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Input")
	bool bActivateOnInputRelease = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Preview")
	TSubclassOf<ANF_SkillPreviewActor> PreviewActorClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Execution")
	ENF_SkillExecutionMode SkillExecutionMode = ENF_SkillExecutionMode::Instant;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Execution", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CastTime = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Execution", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "SkillExecutionMode == ENF_SkillExecutionMode::Timed"))
	float ExecutionDuration = 0.0f;
private:
	friend class UNF_ActiveSkillDataAsset;
	friend class FNF_SkillCastMontageTest;
	friend class FNF_EMPTest;
	friend class FNF_SkillDeathBlockTest;
	void PlayCastMontage(const FGameplayAbilityActorInfo* ActorInfo);
	void CheckLocalInputValidity();
	void InterruptForEMP();
	bool UsesProjectileExecution() const;
	void StartAutomaticExecution();
	void FinishAutomaticExecution();
	bool bCastMontageRequested = false;
	bool bLocalInputHolding = false;
	FTimerHandle LocalInputTimer;
	FTimerHandle AutomaticCastTimer;
	FTimerHandle AutomaticEndTimer;
	UPROPERTY(Transient)
	TObjectPtr<ANF_SkillExecutionActor> ActiveExecutionActor;
	UPROPERTY(Transient)
	FGameplayAbilityTargetDataHandle ConfirmedTargetData;
	UPROPERTY(Transient)
	TObjectPtr<ANF_SkillPreviewActor> LocalPreviewActor;
};
