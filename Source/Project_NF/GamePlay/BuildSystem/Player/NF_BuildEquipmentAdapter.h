// NF_BuildEquipmentAdapter.h
#pragma once

#include "CoreMinimal.h"
#include "GamePlay/BuildSystem/Core/NF_BuildTypes.h"
#include "GamePlay/Combat/Armor/NF_ArmorTypes.h"

class ANF_ArmorBase;
class UNF_BuildItemDefinitionDataAsset;

class PROJECT_NF_API FNF_BuildEquipmentAdapter
{
public:
	static ENF_ArmorSlot ResolveArmorSlot(
		ENF_BuildArmorPart Part);

	static bool IsArmorDefinitionCompatible(
		const UNF_BuildItemDefinitionDataAsset* Definition);
};