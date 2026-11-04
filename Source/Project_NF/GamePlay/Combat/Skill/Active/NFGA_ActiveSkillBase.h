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
	UFUNCTION(BlueprintPure, Category = "NF|Skill|Definition")
	const UNF_ActiveSkillDataAsset* GetActiveData() const;
	virtual bool BeginLocalInput() override;
	virtual bool ConfirmLocalInput(FGameplayAbilityTargetDataHandle& OutTargetData) override;
	virtual void CancelLocalInput() override;
	virtual bool IsLocalInputHolding() const override { return bLocalInputHolding; }
	virtual bool ShouldActivateOnInputRelease() const override { return GetActiveSettings().bActivateOnInputRelease; }
	virtual bool ShouldEndOnInputRelease() const override;
	virtual void ReleaseSkillInput() override;
	virtual bool PrepareConfirmedTargetData(const FGameplayAbilityTargetDataHandle& TargetData) override;
	virtual void ClearConfirmedTargetData() override;
	virtual FGameplayAbilityTargetDataHandle GetConfirmedTargetData() const override { return ConfirmedTargetData; }
	virtual ENF_SkillExecutionMode GetSkillExecutionMode() const override { return GetActiveSettings().ExecutionMode; }
	virtual float GetCastTime() const override { return GetActiveSettings().CastTime; }
	virtual float GetExecutionDuration() const override { return RuntimeExecutionDuration >= 0.f ? RuntimeExecutionDuration : GetActiveSettings().ExecutionDuration; }
	virtual bool CanContinueSkillExecution() const override;
	virtual bool IsAffectedByEMP() const override;
	virtual bool IsBlockedByEMP() const override;
	virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
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
	const UNF_ActiveSkillDataAsset& GetActiveSettings() const;
	UNF_ActiveSkillDataAsset& GetMutableActiveDefaults();
	void SetRuntimeExecutionDuration(float Duration);
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual void CommitExecute(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo) override;
	UFUNCTION(BlueprintImplementableEvent, Category = "NF|Skill|Input")
	void LocalInputCanceled();
private:
	friend class FNF_SkillCastMontageTest;
	friend class FNF_EMPTest;
	friend class FNF_SkillDeathBlockTest;
	void PlayCastMontage(const FGameplayAbilityActorInfo* ActorInfo);
	void CheckLocalInputValidity();
	bool UsesProjectileExecution() const;
	void StartAutomaticExecution();
	void FinishAutomaticExecution();
	float RuntimeExecutionDuration = -1.f;
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
