#pragma once

#include "CoreMinimal.h"
#include "GamePlay/BuildSystem/Core/NF_BuildTypes.h"
#include "GamePlay/BuildSystem/Core/NF_BuildItemSource.h"

class UNF_BuildRules;

/** Pure validation shared by UI previews and authoritative application. */
class PROJECT_NF_API FNF_BuildValidator
{
public:
	static FNF_BuildValidation Validate(
		const UNF_BuildRules* Rules,
		int32 CostLimit,
		const TArray<FNF_BuildSelection>& Selections,
		TMap<FName, FNF_BuildItemData>* OutSelectedData = nullptr,
		// 이미 장착한 빌드. 여기 있는 항목은 코스트에 포함하지 않는다(상점 방문마다 코스트를 새로 쓴다).
		const TArray<FNF_BuildSelection>* OwnedSelections = nullptr);

	// 기존 호출 호환용. 전체 Catalog 공개 정책으로 AllowedItemIds는 사용하지 않는다.
	static FNF_BuildValidation Validate(
		const UNF_BuildRules* Rules,
		int32 CostLimit,
		const TArray<FName>& AllowedItemIds,
		const TArray<FNF_BuildSelection>& Selections,
		TMap<FName, FNF_BuildItemData>* OutSelectedData = nullptr,
		// 이미 장착한 빌드. 여기 있는 항목은 코스트에 포함하지 않는다(상점 방문마다 코스트를 새로 쓴다).
		const TArray<FNF_BuildSelection>* OwnedSelections = nullptr)
	{
		return Validate(Rules, CostLimit, Selections, OutSelectedData, OwnedSelections);
	}

};
