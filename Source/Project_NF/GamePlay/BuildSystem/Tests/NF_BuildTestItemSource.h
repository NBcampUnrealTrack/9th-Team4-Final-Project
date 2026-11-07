#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GamePlay/BuildSystem/Core/NF_BuildTypes.h"
#include "GamePlay/Combat/Skill/Active/NFGA_ActiveSkillBase.h"
#include "GamePlay/Combat/Skill/Data/NF_SkillDataAsset.h"
#include "GamePlay/Combat/Skill/Passive/NFGA_PassiveSkillBase.h"
#include "NF_BuildTestItemSource.generated.h"

/** 자동화 테스트가 원본 담당자의 DA를 대신한다. 저장 가능한 게임 데이터가 아니다. */
UCLASS(Transient, NotBlueprintable)
class UNF_BuildTestItemSource : public UDataAsset, public INF_BuildItemSource
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient)
	FNF_BuildItemData Data;

	bool bAvailable = true;

	virtual bool GetBuildItemData_Implementation(FNF_BuildItemData& OutData) const override
	{
		OutData = bAvailable ? Data : FNF_BuildItemData();
		return bAvailable;
	}
};

// @Owner: 황준현 | 2026-11-06. 빌드는 스킬을 스킬 DA의 GrantTo로만 부여한다.
// 테스트 스킬도 실제 스킬 DA 계열이어야 하며, 빌드 조회값은 테스트가 바꿀 수 있게 따로 둔다.
UCLASS(Transient, NotBlueprintable)
class UNF_BuildTestActiveSkillSource : public UNF_ActiveSkillDataAsset
{
	GENERATED_BODY()

public:
	UNF_BuildTestActiveSkillSource() { AbilityClass = UNFGA_ActiveSkillBase::StaticClass(); }

	UPROPERTY(Transient)
	FNF_BuildItemData Data;

	bool bAvailable = true;

	virtual bool GetBuildItemData_Implementation(FNF_BuildItemData& OutData) const override
	{
		OutData = bAvailable ? Data : FNF_BuildItemData();
		return bAvailable;
	}
};

UCLASS(Transient, NotBlueprintable)
class UNF_BuildTestPassiveSkillSource : public UNF_PassiveSkillDataAsset
{
	GENERATED_BODY()

public:
	UNF_BuildTestPassiveSkillSource() { AbilityClass = UNFGA_PassiveSkillBase::StaticClass(); }

	UPROPERTY(Transient)
	FNF_BuildItemData Data;

	bool bAvailable = true;

	virtual bool GetBuildItemData_Implementation(FNF_BuildItemData& OutData) const override
	{
		OutData = bAvailable ? Data : FNF_BuildItemData();
		return bAvailable;
	}
};
