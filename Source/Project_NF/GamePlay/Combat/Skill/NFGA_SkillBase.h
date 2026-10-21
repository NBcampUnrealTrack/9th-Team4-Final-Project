#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GAS/Foundation/NF_SkillTargetData.h"
#include "GamePlay/Combat/Skill/Preview/NF_SkillPreviewData.h"
#include "NFGA_SkillBase.generated.h"

class ANF_SkillPreviewActor;
class ANF_SkillExecutionActor;

// @Owner: 스킬 담당 | 2026-10-12
// 목적: 장착 유형과 기능 분류를 분리하여 공통 스킬 GA의 설정 정보를 제공한다.
UENUM(BlueprintType)
enum class ENF_SkillType : uint8
{
	Active UMETA(DisplayName = "Active"),
	Passive UMETA(DisplayName = "Passive")
};

UENUM(BlueprintType)
enum class ENF_SkillCategory : uint8
{
	Attack UMETA(DisplayName = "Attack"),
	Utility UMETA(DisplayName = "Utility"),
	Movement UMETA(DisplayName = "Movement"),
	Search UMETA(DisplayName = "Search")
};

UENUM(BlueprintType)
enum class ENF_SkillActivationMethod : uint8
{
	Input UMETA(DisplayName = "Input"),
	OnGranted UMETA(DisplayName = "On Granted"),
	GameplayEvent UMETA(DisplayName = "Gameplay Event"),
	Toggle UMETA(DisplayName = "Toggle")
};

UENUM(BlueprintType)
enum class ENF_SkillExecutionMode : uint8
{
	Instant UMETA(DisplayName = "Instant"),
	Timed UMETA(DisplayName = "Timed"),
	Continuous UMETA(DisplayName = "Continuous")
};

/**
 * @Owner: 스킬 담당 | 2026-10-12
 * 목적: 개별 스킬이 상속하는 공통 Gameplay Ability 베이스.
 *
 * ExecutionActorClass를 지정하면 공통 실행 액터를 자동 생성/시작/정리한다.
 * 자동 실행은 Instant(액터 독립 유지), Timed(시간 종료), Continuous(입력 해제 종료)를 지원한다.
 * 홀딩/확정/취소, 목표 전달, 장착 패시브의 GE/이벤트 수명을 공통으로 관리한다.
 * 실행 클래스가 없는 기존 BP/C++ 자식은 자신의 실행 흐름을 유지한다.
 * 사용 확정 시 CommitAbility의 성공 여부를 확인하고, 완료 시 EndAbility를 호출한다.
 * 취소/종료 시 정리는 GAS의 OnEndAbility 이벤트에서 처리한다.
 * 지속 효과와 중첩은 GE가 관리하며, GA 실행 시간과 독립적이다.
 */
UCLASS(Abstract, Blueprintable)
class PROJECT_NF_API UNFGA_SkillBase : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UNFGA_SkillBase();
	bool ShouldEndOnInputRelease() const;
	void ReleaseSkillInput();
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	// @Owner: 스킬 담당 (구현: Codex) | 2026-10-13
	// 홀딩은 GAS 활성화/비용과 독립적이며, 표시 액터 없이도 사용한다.
	UFUNCTION(BlueprintCallable, Category = "NF|Skill|Input")
	bool BeginLocalInput();
	UFUNCTION(BlueprintCallable, Category = "NF|Skill|Input")
	bool ConfirmLocalInput(FGameplayAbilityTargetDataHandle& OutTargetData);
	UFUNCTION(BlueprintCallable, Category = "NF|Skill|Input")
	void CancelLocalInput();
	UFUNCTION(BlueprintPure, Category = "NF|Skill|Input")
	bool IsLocalInputHolding() const { return bLocalInputHolding; }

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NF|Skill|Targeting")
	bool BuildConfirmedTargetData(FGameplayAbilityTargetDataHandle& OutTargetData) const;
	virtual bool BuildConfirmedTargetData_Implementation(FGameplayAbilityTargetDataHandle& OutTargetData) const;
	UFUNCTION(BlueprintNativeEvent, Category = "NF|Skill|Targeting")
	bool ValidateConfirmedTargetData(const FGameplayAbilityTargetDataHandle& TargetData) const;
	virtual bool ValidateConfirmedTargetData_Implementation(const FGameplayAbilityTargetDataHandle& TargetData) const;
	UFUNCTION(BlueprintPure, Category = "NF|Skill|Targeting")
	FGameplayAbilityTargetDataHandle GetConfirmedTargetData() const { return ConfirmedTargetData; }
	UFUNCTION(BlueprintPure, Category = "NF|Skill|Targeting")
	static FGameplayAbilityTargetDataHandle MakeSkillTargetData(ENF_SkillTargetKind Kind, FVector Location, FVector AimDirection);
	UFUNCTION(BlueprintPure, Category = "NF|Skill|Targeting")
	static bool ReadSkillTargetData(const FGameplayAbilityTargetDataHandle& TargetData, ENF_SkillTargetKind& Kind, FVector& Location, FVector& AimDirection);
	bool PrepareConfirmedTargetData(const FGameplayAbilityTargetDataHandle& TargetData);
	void ClearConfirmedTargetData();
	UFUNCTION(BlueprintPure, Category = "NF|Skill")
	bool CanContinueSkillExecution() const;
	// Active NF skills only. Weapon/basic movement abilities outside this hierarchy are unaffected.
	bool IsAffectedByEMP() const { return SkillType == ENF_SkillType::Active && bAffectedByEMP; }
	bool IsBlockedByEMP() const;

	// 지속/무한 자기 GE만 추적한다. 공유 스택 GE는 전용 시스템에서 수명을 관리한다.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Skill|Passive")
	void TrackPassiveEffect(FActiveGameplayEffectHandle EffectHandle);
	// @Owner: 이정준 (구현: Codex) | 2026-10-22 | Call only after the passive's own condition succeeds.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Skill|Passive|Visual")
	void NotifyPassiveTriggered(AActor* AffectedActor, FVector Location, float Magnitude);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Skill|Passive|Visual")
	void StartPassiveActiveCue();
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Skill|Passive|Visual")
	void StopPassiveActiveCue();
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

	// @Owner: 스킬 담당 (구현: Codex) | 2026-10-13
	// 부여된 InstancedPerActor GA 인스턴스에서 호출한다. 조준 표시는 비용을 소모하지 않는다.
	UFUNCTION(BlueprintCallable, Category = "NF|Skill|Preview")
	bool BeginLocalPreview();

	UFUNCTION(BlueprintCallable, Category = "NF|Skill|Preview")
	void EndLocalPreview();

	// 스킬별 GA/BP가 값만 제공하고 표시 조합은 공통 프리뷰 액터 BP에서 수행한다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NF|Skill|Preview")
	bool GetSkillPreviewData(AActor* SourceAvatar, FNF_SkillPreviewData& OutData) const;
	virtual bool GetSkillPreviewData_Implementation(AActor* SourceAvatar, FNF_SkillPreviewData& OutData) const;

	UFUNCTION(BlueprintPure, Category = "NF|Skill|Input")
	bool ShouldActivateOnInputRelease() const { return bActivateOnInputRelease; }

	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

	UFUNCTION(BlueprintPure, Category = "NF|Skill")
	ENF_SkillType GetSkillType() const { return SkillType; }

	UFUNCTION(BlueprintPure, Category = "NF|Skill")
	ENF_SkillCategory GetSkillCategory() const { return SkillCategory; }

	UFUNCTION(BlueprintPure, Category = "NF|Skill")
	ENF_SkillActivationMethod GetSkillActivationMethod() const { return SkillActivationMethod; }

	UFUNCTION(BlueprintPure, Category = "NF|Skill")
	int32 GetBuildCost() const { return BuildCost; }

	UFUNCTION(BlueprintPure, Category = "NF|Skill")
	ENF_SkillExecutionMode GetSkillExecutionMode() const { return SkillExecutionMode; }

	UFUNCTION(BlueprintPure, Category = "NF|Skill")
	float GetCastTime() const { return CastTime; }

	UFUNCTION(BlueprintPure, Category = "NF|Skill")
	float GetExecutionDuration() const { return ExecutionDuration; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Passive|Visual", meta = (Categories = "GameplayCue"))
	FGameplayTag ActivationCueTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Passive|Visual", meta = (Categories = "GameplayCue"))
	FGameplayTag ActiveCueTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|EMP")
	bool bAffectedByEMP = true;
	// False permits an already-running skill to finish, but still prevents new casts while sealed.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|EMP", meta = (EditCondition = "bAffectedByEMP"))
	bool bCancelOnEMP = true;
	// 지정하면 공통 베이스가 생성/초기화/Commit/시전 대기/종료를 관리한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Execution")
	TSubclassOf<ANF_SkillExecutionActor> ExecutionActorClass;
	// 일반 조합 GA의 프리뷰 값. 기존 수류탄/폭격기는 자신의 값을 제공한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Preview")
	FNF_SkillPreviewData DefaultPreviewData;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	UFUNCTION(BlueprintImplementableEvent, Category = "NF|Skill|Input")
	void LocalInputCanceled();
	UFUNCTION(BlueprintImplementableEvent, Category = "NF|Skill|Passive")
	void PassiveEquipped();
	UFUNCTION(BlueprintImplementableEvent, Category = "NF|Skill|Passive")
	void PassiveUnequipped();
	UFUNCTION(BlueprintImplementableEvent, Category = "NF|Skill|Passive")
	void PassiveGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload);

	// 팀에서 사용하는 사망/기절/침묵 태그를 지정한다. 홀딩과 취소 가능한 활성 GA에 적용한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Input")
	FGameplayTagContainer InputCancelTags;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Targeting")
	bool bRequiresConfirmedTargetData = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Passive")
	TArray<TSubclassOf<UGameplayEffect>> PassiveEffects;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Passive")
	FGameplayTagContainer PassiveEventTags;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Passive")
	bool bActivatePassiveOnGrant = false;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Input")
	bool bActivateOnInputRelease = false;

	// 자식 표시 액터 BP에 필요한 포물선/장판 등 컴포넌트를 부착하여 조합한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Preview")
	TSubclassOf<ANF_SkillPreviewActor> PreviewActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Classification")
	ENF_SkillType SkillType = ENF_SkillType::Active;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Classification")
	ENF_SkillCategory SkillCategory = ENF_SkillCategory::Utility;

	// 실제 발동 연결은 입력/장착 시스템 또는 GAS Ability Triggers에서 설정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Activation")
	ENF_SkillActivationMethod SkillActivationMethod = ENF_SkillActivationMethod::Input;

	// GA 자체의 실행 방식. 버프/디버프의 지속시간은 GE에서 별도로 설정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Execution")
	ENF_SkillExecutionMode SkillExecutionMode = ENF_SkillExecutionMode::Instant;

	// 실행 전 준비 시간(초). 자식 GA에서 AbilityTask 등을 사용해 대기를 구성한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Execution", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CastTime = 0.0f;

	// 자동 Timed 실행의 시간(초). Continuous는 입력 해제 또는 실행 액터 종료로 끝난다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Execution", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "SkillExecutionMode == ENF_SkillExecutionMode::Timed"))
	float ExecutionDuration = 0.0f;

	// 장착 예산용 값. 사용 자원과 쿨타임은 GAS의 Cost/Cooldown GE를 사용한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Build", meta = (ClampMin = "0", UIMin = "0"))
	int32 BuildCost = 0;

private:
	friend class FNF_BerserkerTest;
	TWeakObjectPtr<UAbilitySystemComponent> PassiveCueASC;
	FGameplayTag RegisteredActiveCueTag;
	friend class FNF_EMPTest;
	friend class FNF_SkillDeathBlockTest;
	bool UsesProjectileExecution() const;
	void StartAutomaticExecution();
	void FinishAutomaticExecution();
	FTimerHandle AutomaticCastTimer;
	FTimerHandle AutomaticEndTimer;
	UPROPERTY(Transient)
	TObjectPtr<ANF_SkillExecutionActor> ActiveExecutionActor;
	void CheckLocalInputValidity();
	void BindCancellationTags();
	void UnbindCancellationTags();
	void OnCancellationTagChanged(FGameplayTag Tag, int32 NewCount);
	void InterruptForEMP();
	void InitializePassive();
	void CleanupPassive();
	void HandlePassiveGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload);
	bool bLocalInputHolding = false;
	bool bPassiveInitialized = false;
	FTimerHandle LocalInputTimer;
	TMap<FGameplayTag, FDelegateHandle> CancellationDelegates;
	FDelegateHandle PassiveEventDelegate;
	FGameplayTagContainer RegisteredPassiveEventTags;
	TWeakObjectPtr<UAbilitySystemComponent> RegisteredASC;
	TWeakObjectPtr<AActor> RegisteredAvatar;
	TWeakObjectPtr<AActor> PassiveAvatar;
	TArray<FActiveGameplayEffectHandle> PassiveEffectHandles;
	UPROPERTY(Transient)
	FGameplayAbilityTargetDataHandle ConfirmedTargetData;
	UPROPERTY(Transient)
	TObjectPtr<ANF_SkillPreviewActor> LocalPreviewActor;
};
