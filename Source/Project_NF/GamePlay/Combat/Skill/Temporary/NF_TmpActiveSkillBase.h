#pragma once
#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/Temporary/NF_TmpSkillBase.h"
#include "NF_TmpActiveSkillBase.generated.h"

class UNF_TmpActiveSkillDataAsset;

// @Owner: 이정준 (구현: Codex) | 2026-10-30 | 입력·프리뷰·실행 액터는 액티브만 소유.
UCLASS(Blueprintable)
class PROJECT_NF_API UNF_TmpActiveSkillBase : public UNF_TmpSkillBase
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category = "Tmp Skill")
	const UNF_TmpActiveSkillDataAsset* GetActiveData() const;
	UFUNCTION(BlueprintCallable, Category = "Tmp Skill|Input")
	bool BeginLocalInput();
	// 목표를 만들 뿐 활성화 요청/RPC를 보내지는 않는다. 기존 입력 경로와 연결하지 않는다.
	UFUNCTION(BlueprintCallable, Category = "Tmp Skill|Input")
	bool ConfirmLocalInput(FGameplayAbilityTargetDataHandle& OutTargetData);
	UFUNCTION(BlueprintCallable, Category = "Tmp Skill|Input")
	void CancelLocalInput();
	UFUNCTION(BlueprintPure, Category = "Tmp Skill|Input")
	bool IsHoldingInput() const { return bHoldingInput; }
	virtual void InputReleased(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo) override;
	UFUNCTION(BlueprintNativeEvent, Category = "Tmp Skill|Target")
	bool BuildTargetData(FGameplayAbilityTargetDataHandle& OutTargetData) const;
	virtual bool BuildTargetData_Implementation(FGameplayAbilityTargetDataHandle& OutTargetData) const;
	UFUNCTION(BlueprintNativeEvent, Category = "Tmp Skill|Target")
	bool ValidateTargetData(const FGameplayAbilityTargetDataHandle& TargetData) const;
	virtual bool ValidateTargetData_Implementation(const FGameplayAbilityTargetDataHandle& TargetData) const;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
protected:
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual void HandleRestrictedState() override;
	UFUNCTION(BlueprintImplementableEvent, Category = "Tmp Skill|Preview")
	void PreviewCreated(AActor* CreatedPreviewActor);
	// 서버 전용 효과 연결 지점. 범위 표시/판정/피해는 이 클래스에 넣지 않는다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Tmp Skill|Execution")
	void ExecuteSkill(AActor* SpawnedActor, const FGameplayAbilityTargetDataHandle& TargetData);
private:
	void CheckInputState();
	UPROPERTY(Transient)
	TObjectPtr<AActor> PreviewActor;
	UPROPERTY(Transient)
	TObjectPtr<AActor> ExecutionActor;
	FTimerHandle InputTimer;
	bool bHoldingInput = false;
	bool bOwnsExecutionActor = false;
};
