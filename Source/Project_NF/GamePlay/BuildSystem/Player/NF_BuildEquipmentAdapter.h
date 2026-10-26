#pragma once

#include "CoreMinimal.h"
#include "GamePlay/BuildSystem/Core/NF_BuildItemSource.h"

class PROJECT_NF_API FNF_BuildEquipmentAdapter
{
public:
	static bool IsArmorDefinitionCompatible(const FNF_BuildItemData& Data);
};
