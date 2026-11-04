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
// 공통 설정의 유일한 원본. GA는 이 정의를 직접 조회한다.
UCLASS(BlueprintType)
class PROJECT_NF_API UNF_SkillDataAsset : public UDataAsset, public INF_BuildItemSource
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Skill|Definition")
	ENF_SkillType SkillType = ENF_SkillType::Active;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Definition")
	ENF_SkillActivationMethod SkillActivationMethod = ENF_SkillActivationMethod::Input;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Source", meta = (ToolTip = "이 항목으로 장착할 스킬 GA입니다. 타입, 코스트와 실행 설정은 이 정의가 소유합니다."))
	TSubclassOf<UNFGA_SkillBase> AbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Source", meta = (ClampMin = "1", ToolTip = "빌드에서 스킬 GA를 부여할 때 사용할 어빌리티 레벨입니다."))
	int32 AbilityLevel = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Display", meta = (ToolTip = "이 스킬 항목의 원본 아이콘입니다. 미지정(None)을 허용하며 대체 이미지나 숨김 처리는 UI에서 담당합니다."))
	TObjectPtr<UTexture2D> Icon = nullptr;

	UFUNCTION(BlueprintPure, Category = "NF|Skill|Display")
	UTexture2D* GetSkillIcon() const { return Icon.Get(); }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Definition")
	ENF_SkillCategory SkillCategory = ENF_SkillCategory::Utility;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Definition", meta = (ClampMin = "0"))
	int32 BuildCost = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|State Restrictions")
	TArray<FNF_SkillStateRule> StateRules;

	UFUNCTION(BlueprintPure, Category = "NF|Skill|Definition")
	bool HasValidSettings() const;
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Skill|Definition")
	FGameplayAbilitySpecHandle GrantTo(UAbilitySystemComponent* ASC, int32 InputID = -1) const;

	virtual bool GetBuildItemData_Implementation(FNF_BuildItemData& OutData) const override;
};

UCLASS(BlueprintType)
class PROJECT_NF_API UNF_ActiveSkillDataAsset : public UNF_SkillDataAsset
{
	GENERATED_BODY()
public:
	UNF_ActiveSkillDataAsset();
	// 기존 보스 호출부의 컴파일 호환용. 설정 적용 여부에는 사용하지 않으며 항상 false로 유지.
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "설정은 항상 DA에서 조회합니다. 보스의 구형 guard 제거 후 이 필드를 삭제하세요."))
	bool bOverrideActiveSettings = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Active")
	bool bActivateOnInputRelease = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Active")
	TSubclassOf<ANF_SkillPreviewActor> PreviewActorClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Active")
	FNF_SkillPreviewData PreviewData;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Active")
	TSubclassOf<ANF_SkillExecutionActor> ExecutionActorClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Active")
	ENF_SkillExecutionMode ExecutionMode = ENF_SkillExecutionMode::Instant;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Active", meta = (ClampMin = "0"))
	float CastTime = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Active", meta = (ClampMin = "0"))
	float ExecutionDuration = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Active")
	bool bRequiresConfirmedTargetData = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Active")
	TObjectPtr<class UAnimMontage> CastMontage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Active", meta = (ClampMin = "0.01"))
	float CastMontagePlayRate = 1.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Active")
	FName CastMontageStartSection = NAME_None;
};

UCLASS(BlueprintType)
class PROJECT_NF_API UNF_PassiveSkillDataAsset : public UNF_SkillDataAsset
{
	GENERATED_BODY()
public:
	UNF_PassiveSkillDataAsset();
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Passive")
	TArray<TSubclassOf<UGameplayEffect>> PassiveEffects;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Passive")
	FGameplayTagContainer PassiveEventTags;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Passive")
	bool bActivatePassiveOnGrant = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Passive", meta = (Categories = "GameplayCue"))
	FGameplayTag ActivationCueTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Passive", meta = (Categories = "GameplayCue"))
	FGameplayTag ActiveCueTag;
};
