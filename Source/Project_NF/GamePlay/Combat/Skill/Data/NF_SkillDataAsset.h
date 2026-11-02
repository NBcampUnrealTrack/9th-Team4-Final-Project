#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GamePlay/BuildSystem/Core/NF_BuildItemSource.h"
#include "GamePlay/Combat/Skill/NF_SkillTypes.h"
#include "GamePlay/Combat/Skill/Preview/NF_SkillPreviewData.h"
#include "GameplayAbilitySpec.h"
#include "NF_SkillDataAsset.generated.h"

class UNFGA_SkillBase;
class UTexture2D;
class UAbilitySystemComponent;
class ANF_SkillPreviewActor;
class ANF_SkillExecutionActor;
class UGameplayEffect;

// @Owner: 이정준 (구현: Codex) | 2026-10-26
// 스킬 정의 원본 및 빌드 조회 계약. 기존 DA는 GA 기본값으로 동작한다.
UCLASS(BlueprintType)
class PROJECT_NF_API UNF_SkillDataAsset : public UDataAsset, public INF_BuildItemSource
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Source", meta = (ToolTip = "이 항목으로 장착할 스킬 GA입니다. 종류는 GA에서, 코스트는 Definition 설정 또는 기존 GA 기본값에서 조회합니다."))
	TSubclassOf<UNFGA_SkillBase> AbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Source", meta = (ClampMin = "1", ToolTip = "빌드에서 스킬 GA를 부여할 때 사용할 어빌리티 레벨입니다."))
	int32 AbilityLevel = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Display", meta = (ToolTip = "이 스킬 항목의 원본 아이콘입니다. 미지정(None)을 허용하며 대체 이미지나 숨김 처리는 UI에서 담당합니다."))
	TObjectPtr<UTexture2D> Icon = nullptr;

	UFUNCTION(BlueprintPure, Category = "NF|Skill|Display")
	UTexture2D* GetSkillIcon() const { return Icon.Get(); }

	// 기존 DA는 false를 유지하여 GA의 기존 설정을 그대로 사용한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Definition")
	bool bOverrideIdentity = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Definition", meta = (EditCondition = "bOverrideIdentity"))
	ENF_SkillCategory SkillCategory = ENF_SkillCategory::Utility;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Definition", meta = (ClampMin = "0", EditCondition = "bOverrideIdentity"))
	int32 BuildCost = 0;
	// 기존 사망/EMP/InputCancelTags 정책에 추가한다. 기존 정책을 제거하지 않는다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|State Restrictions")
	TArray<FNF_SkillStateRule> StateRules;

	UFUNCTION(BlueprintPure, Category = "NF|Skill|Definition")
	bool HasValidSettings() const;
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Skill|Definition")
	FGameplayAbilitySpecHandle GrantTo(UAbilitySystemComponent* ASC, int32 InputID = -1) const;
	// 호출 대상은 부여된 GA 인스턴스만 허용한다. CDO는 수정하지 않는다.
	virtual void ConfigureInstance(UNFGA_SkillBase& Skill) const;

	virtual bool GetBuildItemData_Implementation(FNF_BuildItemData& OutData) const override;
};

UCLASS(BlueprintType)
class PROJECT_NF_API UNF_ActiveSkillDataAsset : public UNF_SkillDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Active")
	bool bOverrideActiveSettings = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Active", meta = (EditCondition = "bOverrideActiveSettings"))
	bool bActivateOnInputRelease = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Active", meta = (EditCondition = "bOverrideActiveSettings"))
	TSubclassOf<ANF_SkillPreviewActor> PreviewActorClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Active", meta = (EditCondition = "bOverrideActiveSettings"))
	FNF_SkillPreviewData PreviewData;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Active", meta = (EditCondition = "bOverrideActiveSettings"))
	TSubclassOf<ANF_SkillExecutionActor> ExecutionActorClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Active", meta = (EditCondition = "bOverrideActiveSettings"))
	ENF_SkillExecutionMode ExecutionMode = ENF_SkillExecutionMode::Instant;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Active", meta = (ClampMin = "0", EditCondition = "bOverrideActiveSettings"))
	float CastTime = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Active", meta = (ClampMin = "0", EditCondition = "bOverrideActiveSettings"))
	float ExecutionDuration = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Active", meta = (EditCondition = "bOverrideActiveSettings"))
	bool bRequiresConfirmedTargetData = false;
	virtual void ConfigureInstance(UNFGA_SkillBase& Skill) const override;
};

UCLASS(BlueprintType)
class PROJECT_NF_API UNF_PassiveSkillDataAsset : public UNF_SkillDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Passive")
	bool bOverridePassiveSettings = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Passive", meta = (EditCondition = "bOverridePassiveSettings"))
	TArray<TSubclassOf<UGameplayEffect>> PassiveEffects;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Passive", meta = (EditCondition = "bOverridePassiveSettings"))
	FGameplayTagContainer PassiveEventTags;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Passive", meta = (EditCondition = "bOverridePassiveSettings"))
	bool bActivatePassiveOnGrant = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Passive", meta = (Categories = "GameplayCue", EditCondition = "bOverridePassiveSettings"))
	FGameplayTag ActivationCueTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Passive", meta = (Categories = "GameplayCue", EditCondition = "bOverridePassiveSettings"))
	FGameplayTag ActiveCueTag;
	virtual void ConfigureInstance(UNFGA_SkillBase& Skill) const override;
};
