#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GamePlay/BuildSystem/Core/NF_BuildItemSource.h"
#include "NF_SkillDataAsset.generated.h"

class UNFGA_SkillBase;
class UTexture2D;

// @Owner: 이정준 (구현: Codex) | 2026-10-26
// 스킬 아이콘 원본 및 빌드의 원본 조회 인터페이스. 종류/코스트는 GA 기본값에서 읽는다.
UCLASS(BlueprintType)
class PROJECT_NF_API UNF_SkillDataAsset : public UDataAsset, public INF_BuildItemSource
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Source", meta = (ToolTip = "이 항목으로 장착할 스킬 GA입니다. 스킬 종류와 코스트는 이 GA의 기본 설정에서 조회합니다."))
	TSubclassOf<UNFGA_SkillBase> AbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Source", meta = (ClampMin = "1", ToolTip = "빌드에서 스킬 GA를 부여할 때 사용할 어빌리티 레벨입니다."))
	int32 AbilityLevel = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill|Display", meta = (ToolTip = "이 스킬 항목의 원본 아이콘입니다. 미지정(None)을 허용하며 대체 이미지나 숨김 처리는 UI에서 담당합니다."))
	TObjectPtr<UTexture2D> Icon = nullptr;

	UFUNCTION(BlueprintPure, Category = "NF|Skill|Display")
	UTexture2D* GetSkillIcon() const { return Icon.Get(); }

	virtual bool GetBuildItemData_Implementation(FNF_BuildItemData& OutData) const override;
};
