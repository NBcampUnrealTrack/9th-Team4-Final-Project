#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GamePlay/BuildSystem/Core/NF_BuildItemSource.h"
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
