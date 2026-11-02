#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GAS/Foundation/NF_SkillTargetData.h"
#include "GamePlay/Combat/Skill/Data/NF_SkillDataAsset.h"
#include "NFGA_SkillBase.generated.h"

// 공통 책임: DA, 상태 제한, 부여/아바타/제거 수명. 실행 상태는 유형별 자식이 소유한다.
UCLASS(Abstract, Blueprintable)
class PROJECT_NF_API UNFGA_SkillBase : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UNFGA_SkillBase();
	UFUNCTION(BlueprintPure, Category = "NF|Skill|Definition")
	const UNF_SkillDataAsset* GetSkillData() const;
	UFUNCTION(BlueprintPure, Category = "NF|Skill|Definition")
	ENF_SkillType GetSkillType() const;
	UFUNCTION(BlueprintPure, Category = "NF|Skill|Definition")
	ENF_SkillCategory GetSkillCategory() const;
	UFUNCTION(BlueprintPure, Category = "NF|Skill|Definition")
	ENF_SkillActivationMethod GetSkillActivationMethod() const;
	UFUNCTION(BlueprintPure, Category = "NF|Skill|Definition")
	int32 GetBuildCost() const;
	UFUNCTION(BlueprintPure, Category = "NF|Skill")
	virtual bool CanContinueSkillExecution() const;
	virtual bool IsAffectedByEMP() const { return false; }
	virtual bool IsBlockedByEMP() const { return false; }
	virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

	// 기존 ASC/BP 입력 호출용 호환 프로토콜. 공통 클래스는 실행 상태를 소유하지 않는다.
	UFUNCTION(BlueprintCallable, Category = "NF|Skill|Input")
	virtual bool BeginLocalInput() { return false; }
	UFUNCTION(BlueprintCallable, Category = "NF|Skill|Input")
	virtual bool ConfirmLocalInput(FGameplayAbilityTargetDataHandle& OutTargetData);
	UFUNCTION(BlueprintCallable, Category = "NF|Skill|Input")
	virtual void CancelLocalInput() {}
	UFUNCTION(BlueprintPure, Category = "NF|Skill|Input")
	virtual bool IsLocalInputHolding() const { return false; }
	UFUNCTION(BlueprintPure, Category = "NF|Skill|Input")
	virtual bool ShouldActivateOnInputRelease() const { return false; }
	virtual bool ShouldEndOnInputRelease() const { return false; }
	virtual void ReleaseSkillInput() {}
	virtual bool PrepareConfirmedTargetData(const FGameplayAbilityTargetDataHandle& TargetData) { return false; }
	virtual void ClearConfirmedTargetData() {}
	UFUNCTION(BlueprintPure, Category = "NF|Skill|Targeting")
	virtual FGameplayAbilityTargetDataHandle GetConfirmedTargetData() const { return {}; }
	UFUNCTION(BlueprintPure, Category = "NF|Skill|Targeting")
	static FGameplayAbilityTargetDataHandle MakeSkillTargetData(ENF_SkillTargetKind Kind, FVector Location, FVector AimDirection);
	UFUNCTION(BlueprintPure, Category = "NF|Skill|Targeting")
	static bool ReadSkillTargetData(const FGameplayAbilityTargetDataHandle& TargetData, ENF_SkillTargetKind& Kind, FVector& Location, FVector& AimDirection);
	UFUNCTION(BlueprintPure, Category = "NF|Skill")
	virtual ENF_SkillExecutionMode GetSkillExecutionMode() const { return ENF_SkillExecutionMode::Instant; }
	UFUNCTION(BlueprintPure, Category = "NF|Skill")
	virtual float GetCastTime() const { return 0.f; }
	UFUNCTION(BlueprintPure, Category = "NF|Skill")
	virtual float GetExecutionDuration() const { return 0.f; }
protected:
	// 직접 지정하면 기존 빌드 부여 경로도 DA 설정을 사용할 수 있다. Spec의 DA가 우선한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Definition")
	TObjectPtr<UNF_SkillDataAsset> SkillDefinition;
	UNF_SkillDataAsset& GetMutableSkillDefaults();
	bool UsesDefinitionStateRules() const;
	virtual void CollectStateRestrictionTags(FGameplayTagContainer& Tags) const {}
	virtual void HandleStateRestrictionChanged(FGameplayTag Tag, int32 Count) {}
private:
	friend class UNF_SkillDataAsset;
	friend class FNF_EMPTest;
	friend class FNF_SkillDeathBlockTest;
	void ConfigureFromSkillData(const FGameplayAbilitySpec& Spec);
	bool HasBlockingStateRule(const UAbilitySystemComponent* ASC) const;
	void BindCancellationTags();
	void UnbindCancellationTags();
	void OnCancellationTagChanged(FGameplayTag Tag, int32 NewCount);
	// 네이티브 기본 분류도 GA scalar 대신 DA 객체에 보관한다. 외부 DA 미연결 시 사용.
	UPROPERTY(Instanced)
	TObjectPtr<UNF_SkillDataAsset> NativeSkillDefaults;
	UPROPERTY(Transient)
	TObjectPtr<UNF_SkillDataAsset> SkillData;
	TArray<FNF_SkillStateRule> AdditionalStateRules;
	TMap<FGameplayTag, FDelegateHandle> CancellationDelegates;
	TWeakObjectPtr<UAbilitySystemComponent> RegisteredASC;
	TWeakObjectPtr<AActor> RegisteredAvatar;
};
