#include "GamePlay/BuildSystem/Core/NF_BuildValidator.h"

#include "Abilities/GameplayAbility.h"
#include "GamePlay/BuildSystem/Player/NF_BuildEquipmentAdapter.h"
#include "GamePlay/BuildSystem/Data/NF_BuildItemDefinitionDataAsset.h"
#include "GamePlay/BuildSystem/Data/NF_BuildRules.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponent.h"
#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"
#include "GamePlay/Combat/Weapons/NF_WeaponDataAsset.h"
#include "GamePlay/Combat/Armor/NF_ArmorBase.h"

FNF_BuildValidation FNF_BuildValidator::Validate(const UNF_BuildRules* Rules, int32 CostLimit,
                                                 const TArray<FName>& AllowedItemIds,
                                                 const TArray<FNF_BuildSelection>& Selections,
                                                 TMap<FName, FNF_BuildItemData>* OutSelectedData,
                                                 const TArray<FNF_BuildSelection>* OwnedSelections)
{
	if (OutSelectedData) OutSelectedData->Reset();
	FNF_BuildValidation Validation;
	auto Fail = [&Validation](ENF_BuildResult Result, const TCHAR* Message)
	{
		Validation.Result = Result;
		Validation.Message = FText::FromString(Message);
		return Validation;
	};

	if (!IsValid(Rules) || CostLimit < 0)
	{
		return Fail(
			ENF_BuildResult::InvalidRules,
			TEXT("빌드 규칙 또는 코스트 상한이 잘못되었습니다."));
	}
	Validation.CostLimit = CostLimit;

	if (Rules->WeaponSlots != 1 ||
		Rules->ArmorSlots != 3 ||
		Rules->ActiveSkillSlots != 2 ||
		Rules->PassiveSkillSlots != 1)
	{
		return Fail(
			ENF_BuildResult::InvalidRules,
			TEXT("빌드 슬롯 수를 확인하세요."));
	}

	TMap<FName, FNF_BuildItemData> Catalog;
	// 임시: 원본 연결이 깨진 항목 하나 때문에 전체 빌드(시작 무기 포함)가 막히지 않도록,
	// 해석되지 않는 항목은 건너뛰고 해석되는 항목만 인식한다. 건너뛴 항목을 선택하면 InvalidRules로 거절한다.
	TSet<FName> SkippedItemIds;
	for (const UNF_BuildItemDefinitionDataAsset* Definition : Rules->Catalog)
	{
		FNF_BuildItemData Data;
		const FName ItemId = IsValid(Definition) ? Definition->ItemId : NAME_None;
		if (ItemId.IsNone() || SkippedItemIds.Contains(ItemId) || Catalog.Contains(ItemId) ||
			!Rules->ResolveItemData(ItemId, Data) || Data.Cost < 0 ||
			static_cast<uint8>(Data.ItemType) > static_cast<uint8>(ENF_ItemType::PassiveSkill))
		{
			// return Fail(ENF_BuildResult::InvalidRules,
			//             TEXT("카탈로그 ID·원본 연결·코스트·종류를 확인하세요. 중복 ID 또는 미연결 원본은 사용할 수 없습니다."));
			if (!ItemId.IsNone())
			{
				// 중복 ID는 어느 원본이 맞는지 알 수 없으므로 먼저 등록된 항목도 함께 제외한다.
				Catalog.Remove(ItemId);
				SkippedItemIds.Add(ItemId);
			}
			UE_LOG(LogTemp, Warning, TEXT("빌드 카탈로그 항목 제외: ItemId=%s. 원본 연결·코스트·종류·중복 ID를 확인하세요."),
			       *ItemId.ToString());
			continue;
		}
		Catalog.Add(ItemId, Data);
		if (Data.ItemType == ENF_ItemType::Armor && FNF_BuildItemSlots::GetDefaultSlotIndex(Data) == INDEX_NONE)
		{
			return Fail(ENF_BuildResult::InvalidRules, TEXT("방어구 항목의 머리/상의/하의 부위를 지정해야 합니다."));
		}
	}

	if (Selections.Num() > Rules->WeaponSlots + Rules->ArmorSlots + Rules->ActiveSkillSlots + Rules->PassiveSkillSlots)
	{
		return Fail(ENF_BuildResult::InvalidSlot, TEXT("빌드 슬롯 수를 초과했습니다."));
	}

	TSet<FName> OwnedItemIds;
	if (OwnedSelections)
	{
		for (const FNF_BuildSelection& Owned : *OwnedSelections)
		{
			OwnedItemIds.Add(Owned.ItemId);
		}
	}

	TSet<FName> ItemIds;
	TSet<int32> OccupiedSlots;
	int64 TotalCost = 0;
	bool bHasWeapon = false;
	int32 LinkedWeaponCount = 0;
	for (const FNF_BuildSelection& Selection : Selections)
	{
		const FNF_BuildItemData* Found = Catalog.Find(Selection.ItemId);

		if (SkippedItemIds.Contains(Selection.ItemId))
		{
			return Fail(ENF_BuildResult::InvalidRules,
			            TEXT("선택한 항목의 카탈로그 ID·원본 연결·코스트·종류를 확인하세요. 중복 ID 또는 미연결 원본은 사용할 수 없습니다."));
		}

		if (!Found)
		{
			return Fail(ENF_BuildResult::UnknownItem,TEXT("상점 카탈로그에 없는 항목입니다."));
		}

		if (!AllowedItemIds.Contains(Selection.ItemId))
		{
			return Fail(ENF_BuildResult::ItemNotUnlocked,TEXT("아직 획득하지 않은 항목입니다."));
		}

		const FNF_BuildItemData& Data = *Found;
		if (ItemIds.Contains(Selection.ItemId))
		{
			return Fail(ENF_BuildResult::DuplicateItem, TEXT("동일한 항목을 중복 선택할 수 없습니다."));
		}
		ItemIds.Add(Selection.ItemId);

		if (!FNF_BuildItemSlots::CanUseSlot(Data, Selection.SlotIndex) ||
			Selection.SlotIndex >= Rules->GetSlotCount(Data.ItemType))
		{
			return Fail(ENF_BuildResult::InvalidSlot, TEXT("아이템 종류 또는 방어구 부위와 맞지 않는 슬롯입니다."));
		}
		const int32 SlotKey = static_cast<int32>(Data.ItemType) * 8 + Selection.SlotIndex;
		if (OccupiedSlots.Contains(SlotKey))
		{
			return Fail(ENF_BuildResult::InvalidSlot, TEXT("슬롯 범위가 잘못되었거나 이미 사용 중입니다."));
		}
		OccupiedSlots.Add(SlotKey);

		if (Data.WeaponClass)
		{
			if (Data.ItemType != ENF_ItemType::Weapon ||
				Data.WeaponClass->HasAnyClassFlags(CLASS_Abstract) || !IsValid(Data.WeaponData) ||
				Data.WeaponData->ItemType != ENF_ItemType::Weapon ||
				Data.WeaponData->WeaponClass != Data.WeaponClass)
			{
				return Fail(ENF_BuildResult::InvalidGrant, TEXT("선택한 무기 원본 DA와 생성 클래스를 확인하세요."));
			}
			if (Selection.SlotIndex != 0 || ++LinkedWeaponCount > 1)
			{
				return Fail(ENF_BuildResult::InvalidSlot, TEXT("실제 무기 장착은 무기 슬롯 1의 한 항목만 지원합니다."));
			}
		}

		if (Data.ArmorClass && !FNF_BuildEquipmentAdapter::IsArmorDefinitionCompatible(Data))
		{
			return Fail(ENF_BuildResult::InvalidGrant, TEXT("방어구 원본의 종류·부위와 생성 클래스를 확인하세요."));
		}

		if (Data.AbilityLevel < 1 ||
			(Data.AbilityClass && Data.AbilityClass->HasAnyClassFlags(CLASS_Abstract)))
		{
			return Fail(ENF_BuildResult::InvalidGrant, TEXT("부여할 Ability의 클래스 또는 레벨이 잘못되었습니다."));
		}
		for (const TSubclassOf<UGameplayEffect>& EffectClass : Data.EquippedEffects)
		{
			const UGameplayEffect* Effect = EffectClass ? EffectClass->GetDefaultObject<UGameplayEffect>() : nullptr;
			// UE 5.7's GetStackingType lacks a DLL export. Read the public legacy field for this engine version.
			PRAGMA_DISABLE_DEPRECATION_WARNINGS
			const bool bHasStacking = Effect && Effect->StackingType != EGameplayEffectStackingType::None;
			PRAGMA_ENABLE_DEPRECATION_WARNINGS
			if (!Effect || EffectClass->HasAnyClassFlags(CLASS_Abstract) ||
				Effect->DurationPolicy != EGameplayEffectDurationType::Infinite || bHasStacking ||
				Effect->Period.GetValueAtLevel(Data.AbilityLevel) != 0.f || !Effect->Executions.IsEmpty() ||
				Effect->FindComponent<UGameplayEffectComponent>())
			{
				return Fail(ENF_BuildResult::InvalidGrant,
				            TEXT("장착 GE는 Infinite, 비주기, 비중첩 Modifier만 "
					            "지원합니다. GE Component는 사용할 수 없습니다."));
			}
		}

		bHasWeapon |= Data.ItemType == ENF_ItemType::Weapon;
		if (!OwnedItemIds.Contains(Selection.ItemId))
		{
			TotalCost += Data.Cost;
		}
		Validation.TotalCost = static_cast<int32>(FMath::Min<int64>(TotalCost, MAX_int32));
	}

	if (!bHasWeapon)
	{
		return Fail(ENF_BuildResult::RequiredWeaponMissing, TEXT("주무기를 하나 이상 선택해야 합니다."));
	}
	if (TotalCost > CostLimit)
	{
		return Fail(ENF_BuildResult::CostLimitExceeded,TEXT("플레이어의 코스트 상한을 초과했습니다."));
	}
	if (OutSelectedData)
	{
		for (const FNF_BuildSelection& Selection : Selections)
		{
			OutSelectedData->Add(Selection.ItemId, Catalog.FindChecked(Selection.ItemId));
		}
	}
	Validation.Message = FText::FromString(TEXT("적용 가능한 빌드입니다."));
	return Validation;
}
