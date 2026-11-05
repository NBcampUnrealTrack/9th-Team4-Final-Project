#include "GamePlay/BuildSystem/Data/NF_BuildRules.h"
#include "GamePlay/BuildSystem/Data/NF_BuildItemDefinitionDataAsset.h"
#include "GamePlay/BuildSystem/Data/NF_BuildInitialSettings.h"
#include "GamePlay/BuildSystem/Core/NF_BuildValidator.h"
#if WITH_EDITOR
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Misc/DataValidation.h"
#endif
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

bool UNF_BuildRules::HasFixedSlotCounts() const
{
	return WeaponSlots == FixedWeaponSlots && ArmorSlots == FixedArmorSlots &&
		ActiveSkillSlots == FixedActiveSkillSlots && PassiveSkillSlots == FixedPassiveSkillSlots;
}

void UNF_BuildRules::PostLoad()
{
	Super::PostLoad();
	// 편집 가능하던 시기에 다른 값으로 저장된 에셋도 고정 슬롯 구성으로 읽는다.
	if (!HasFixedSlotCounts())
	{
		UE_LOG(LogTemp, Warning,
		       TEXT("빌드 Rules %s의 저장된 슬롯 수(무기 %d, 방어구 %d, 액티브 %d, 패시브 %d)를 고정값으로 되돌립니다. 에셋을 다시 저장하세요."),
		       *GetPathName(), WeaponSlots, ArmorSlots, ActiveSkillSlots, PassiveSkillSlots);
		WeaponSlots = FixedWeaponSlots;
		ArmorSlots = FixedArmorSlots;
		ActiveSkillSlots = FixedActiveSkillSlots;
		PassiveSkillSlots = FixedPassiveSkillSlots;
	}
}

#if WITH_EDITOR
EDataValidationResult UNF_BuildRules::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	auto AddError = [&Context, &Result](const FString& Message)
	{
		Context.AddError(FText::FromString(Message));
		Result = EDataValidationResult::Invalid;
	};

	if (!HasFixedSlotCounts())
	{
		AddError(TEXT("슬롯 수는 무기 1, 방어구 3, 액티브 2, 패시브 1로 고정입니다."));
	}

	// 카탈로그: 중복 ID, 원본 누락·조회 실패, 부위 미지정 방어구. 런타임 검증기는 이런 항목을 조용히 제외한다.
	TSet<FName> SeenItemIds;
	TSet<FName> ReportedDuplicates;
	for (int32 Index = 0; Index < Catalog.Num(); ++Index)
	{
		const UNF_BuildItemDefinitionDataAsset* Definition = Catalog[Index];
		if (!IsValid(Definition))
		{
			AddError(FString::Printf(TEXT("Catalog[%d]이 비어 있습니다."), Index));
			continue;
		}
		const FName ItemId = Definition->ItemId;
		if (ItemId.IsNone())
		{
			AddError(FString::Printf(TEXT("Catalog[%d] %s의 ItemId가 비어 있습니다."), Index, *Definition->GetName()));
			continue;
		}
		if (SeenItemIds.Contains(ItemId))
		{
			if (!ReportedDuplicates.Contains(ItemId))
			{
				AddError(FString::Printf(TEXT("ItemId %s가 Catalog에 중복 등록되어 있습니다. 중복된 항목은 모두 상점에서 거절됩니다."),
				                         *ItemId.ToString()));
				ReportedDuplicates.Add(ItemId);
			}
			continue;
		}
		SeenItemIds.Add(ItemId);

		const TObjectPtr<UDataAsset>* Source = ItemSources.Find(ItemId);
		if (!Source || !IsValid(Source->Get()))
		{
			AddError(FString::Printf(TEXT("ItemId %s의 ItemSources 원본 DA가 없습니다."), *ItemId.ToString()));
			continue;
		}
		FNF_BuildItemData Data;
		if (!ResolveItemData(ItemId, Data))
		{
			AddError(FString::Printf(TEXT("ItemId %s의 원본 %s에서 빌드 정보를 읽지 못했습니다. 원본 DA 설정(코스트·종류·클래스)을 확인하세요."),
			                         *ItemId.ToString(), *GetNameSafe(Source->Get())));
			continue;
		}
		if (Data.ItemType == ENF_ItemType::Armor && FNF_BuildItemSlots::GetDefaultSlotIndex(Data) == INDEX_NONE)
		{
			AddError(FString::Printf(TEXT("방어구 %s의 부위(머리/상의/하의)가 지정되지 않았습니다."), *ItemId.ToString()));
		}
	}
	for (const TPair<FName, TObjectPtr<UDataAsset>>& Pair : ItemSources)
	{
		if (!SeenItemIds.Contains(Pair.Key))
		{
			Context.AddWarning(FText::FromString(FString::Printf(
				TEXT("ItemSources의 %s가 Catalog에 없습니다. 상점에 표시되지 않습니다."), *Pair.Key.ToString())));
		}
	}

	// 기본 빌드: 런타임과 같은 검증기로 구성·슬롯을 검사한다. 코스트는 아래에서 초기 설정과 비교한다.
	const FNF_BuildValidation FallbackValidation = FNF_BuildValidator::Validate(this, MAX_int32, FallbackBuild);
	if (FallbackValidation.Result != ENF_BuildResult::Success)
	{
		AddError(FString::Printf(TEXT("FallbackBuild가 유효하지 않습니다: %s"), *FallbackValidation.Message.ToString()));
	}
	else
	{
		// 초기 설정 DA는 Rules를 참조하지 않으므로 프로젝트의 모든 초기 설정과 비교한다.
		TArray<FAssetData> SettingsAssets;
		IAssetRegistry::GetChecked().GetAssetsByClass(
			UNF_BuildInitialSettings::StaticClass()->GetClassPathName(), SettingsAssets, true);
		for (const FAssetData& Asset : SettingsAssets)
		{
			const UNF_BuildInitialSettings* Settings = Cast<UNF_BuildInitialSettings>(Asset.GetAsset());
			if (IsValid(Settings) && FallbackValidation.TotalCost > Settings->InitialCostLimit)
			{
				AddError(FString::Printf(
					TEXT("FallbackBuild 합계 코스트 %d가 %s의 InitialCostLimit %d보다 큽니다. 매치 시작 기본 빌드가 실패합니다."),
					FallbackValidation.TotalCost, *Settings->GetName(), Settings->InitialCostLimit));
			}
		}
	}
	return Result;
}
#endif

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
