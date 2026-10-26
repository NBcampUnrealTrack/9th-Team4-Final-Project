// Fill out your copyright notice in the Description page of Project Settings.


#include "NF_SkillsViewModel.h"

#include "Data/Enum/NF_ItemTypes.h"
#include "GamePlay/BuildSystem/Core/NF_BuildItemSource.h"
#include "GamePlay/BuildSystem/Data/NF_BuildItemDefinitionDataAsset.h"
#include "GamePlay/Combat/Skill/NFGA_SkillBase.h"

void UNF_SkillsViewModel::Configure(
	const UNF_BuildItemDefinitionDataAsset* Item, const FNF_BuildItemData& ItemData,UTexture2D* InIcon,const FText& InDescription,float InCooldownSeconds)
{
	if (!IsValid(Item)) return;

	if (ItemData.ItemType != ENF_ItemType::ActiveSkill && ItemData.ItemType != ENF_ItemType::PassiveSkill) return;

	UE_MVVM_SET_PROPERTY_VALUE(SkillId, Item->ItemId);
	UE_MVVM_SET_PROPERTY_VALUE(DisplayName, Item->DisplayName);
	UE_MVVM_SET_PROPERTY_VALUE(Cost, ItemData.Cost);
	UE_MVVM_SET_PROPERTY_VALUE(Icon, InIcon);
	UE_MVVM_SET_PROPERTY_VALUE(Description, InDescription);
	UE_MVVM_SET_PROPERTY_VALUE(CooldownSeconds,FMath::Max(0.0f, InCooldownSeconds));
	UE_MVVM_SET_PROPERTY_VALUE(CastTime, ResolveSkillCastTime(ItemData));
}

float UNF_SkillsViewModel::ResolveSkillCastTime(const FNF_BuildItemData& ItemData) const
{
	if (!ItemData.AbilityClass) { return 0.0f; }

	const UNFGA_SkillBase* SkillDefaults =Cast<UNFGA_SkillBase>(ItemData.AbilityClass->GetDefaultObject());

	if (!IsValid(SkillDefaults)) {return 0.0f;}
		
	return SkillDefaults->GetCastTime();
	
}
