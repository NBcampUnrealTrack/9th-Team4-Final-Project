#include "GamePlay/BuildSystem/Data/NF_BuildRules.h"
#include "GamePlay/BuildSystem/Data/NF_BuildItemDefinitionDataAsset.h"
#include "GamePlay/Combat/Armor/NF_ArmorDataAsset.h"
#include "GamePlay/Combat/Weapons/NF_WeaponDataAsset.h"

bool UNF_BuildRules::ResolveItemData(FName ItemId, FNF_BuildItemData& OutData) const
{
	OutData = FNF_BuildItemData();
	const TObjectPtr<UDataAsset>* Source = ItemSources.Find(ItemId);
	if (!Source || !IsValid(Source->Get()) ||
		!Source->Get()->GetClass()->ImplementsInterface(UNF_BuildItemSource::StaticClass()))
	{
		return false;
	}
	FNF_BuildItemData Resolved;
	if (!INF_BuildItemSource::Execute_GetBuildItemData(Source->Get(), Resolved) || Resolved.Cost < 0 ||
		static_cast<uint8>(Resolved.ItemType) > static_cast<uint8>(ENF_ItemType::PassiveSkill))
	{
		return false;
	}
	// 반환자가 임의의 다른 티어를 지정하지 못하도록 실제 조회한 원본으로 고정한다.
	Resolved.WeaponData = Cast<UNF_WeaponDataAsset>(Source->Get());
	Resolved.ArmorData = Cast<UNF_ArmorDataAsset>(Source->Get());
	OutData = MoveTemp(Resolved);
	return true;
}

int32 UNF_BuildRules::GetSlotCount(ENF_ItemType ItemType) const
{
	switch (ItemType)
	{
	case ENF_ItemType::Weapon:
		return WeaponSlots;
	case ENF_ItemType::Armor:
		return ArmorSlots;
	case ENF_ItemType::ActiveSkill:
		return ActiveSkillSlots;
	case ENF_ItemType::PassiveSkill:
		return PassiveSkillSlots;
	default:
		return 0;
	}
}

bool UNF_BuildRules::TryGetCostLimit(int32 PlayerLevel, int32& OutCostLimit) const
{
	OutCostLimit = 0;
	if (PlayerLevel < 1 || !CostLimitsByLevel.IsValidIndex(PlayerLevel - 1))
	{
		return false;
	}

	OutCostLimit = CostLimitsByLevel[PlayerLevel - 1];
	return OutCostLimit >= 0;
}

UNF_BuildItemDefinitionDataAsset* UNF_BuildRules::FindItemDefinition(FName ItemId) const
{
	for (UNF_BuildItemDefinitionDataAsset* Item : Catalog)
	{
		if (IsValid(Item) && Item->ItemId == ItemId) { return Item; }
	}
	return nullptr;
}
