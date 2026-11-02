#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GamePlay/Combat/Skill/Temporary/NF_TmpSkillTypes.h"
#include "GameplayAbilitySpec.h"
#include "NF_TmpSkillDataAsset.generated.h"

class UNF_TmpSkillBase;
class UAbilitySystemComponent;
class UGameplayEffect;
class UTexture2D;
class AActor;

// @Owner: 이정준 (구현: Codex) | 2026-10-30
// 정적 정보는 DA가 원본이다. 기존 NF_SkillDataAsset / 빌드 DA와는 연결하지 않는다.
UCLASS(Abstract, BlueprintType)
class PROJECT_NF_API UNF_TmpSkillDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill|Identity")
	ENF_TmpSkillType SkillType = ENF_TmpSkillType::Active;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Identity")
	ENF_TmpSkillCategory Category = ENF_TmpSkillCategory::Utility;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Identity", meta = (ClampMin = "0"))
	int32 Cost = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Identity")
	TObjectPtr<UTexture2D> Icon;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Ability")
	TSubclassOf<UNF_TmpSkillBase> AbilityClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Ability", meta = (ClampMin = "1"))
	int32 AbilityLevel = 1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|State Restrictions", meta = (ToolTip = "EMP·사망·기절 등의 상태별로 새 사용 차단과 실행 중단을 설정합니다. 태그 자체를 부여하는 기능은 아닙니다."))
	TArray<FNF_TmpSkillStateRule> StateRules;
	UFUNCTION(BlueprintPure, Category = "Tmp Skill")
	bool HasValidSettings() const;
	// 이 DA를 Spec의 SourceObject로 전달한다. GA의 CDO에는 설정을 복사하지 않는다.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Tmp Skill")
	FGameplayAbilitySpecHandle GrantTo(UAbilitySystemComponent* ASC) const;
};

UCLASS(BlueprintType)
class PROJECT_NF_API UNF_TmpActiveSkillDataAsset : public UNF_TmpSkillDataAsset
{
	GENERATED_BODY()
public:
	UNF_TmpActiveSkillDataAsset();
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Input")
	ENF_TmpSkillInput InputMethod = ENF_TmpSkillInput::Press;
	// 로컬 표시 전용. 피해 등 실제 게임플레이 로직을 넣지 않는다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Preview")
	TSubclassOf<AActor> PreviewActorClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Execution")
	TSubclassOf<AActor> ExecutionActorClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Execution", meta = (ToolTip = "켜면 실행 액터가 GA 종료 후에도 남습니다. 수류탄 등에 사용하며 액터에서 자체 수명을 설정해야 합니다."))
	bool bIndependentExecutionActor = true;
};

UCLASS(BlueprintType)
class PROJECT_NF_API UNF_TmpPassiveSkillDataAsset : public UNF_TmpSkillDataAsset
{
	GENERATED_BODY()
public:
	UNF_TmpPassiveSkillDataAsset();
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Equipped Effects")
	TArray<TSubclassOf<UGameplayEffect>> EquippedEffects;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Events")
	FGameplayTagContainer EventTags;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Visual", meta = (Categories = "GameplayCue"))
	FGameplayTag TriggerCueTag;
};
