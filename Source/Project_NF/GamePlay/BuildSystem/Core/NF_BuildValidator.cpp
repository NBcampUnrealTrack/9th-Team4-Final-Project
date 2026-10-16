#include "GamePlay/BuildSystem/Core/NF_BuildValidator.h"

#include "Abilities/GameplayAbility.h"
#include "GamePlay/BuildSystem/Data/NF_BuildItemDefinitionDataAsset.h"
#include "GamePlay/BuildSystem/Data/NF_BuildRules.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponent.h"
#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"

FNF_BuildValidation FNF_BuildValidator::Validate(
	const UNF_BuildRules* Rules, int32 PlayerLevel, const TArray<FNF_BuildSelection>& Selections)
{
	FNF_BuildValidation Validation;
	auto Fail = [&Validation](ENF_BuildResult Result, const TCHAR* Message)
	{
		Validation.Result = Result;
		Validation.Message = FText::FromString(Message);
		return Validation;
	};

	if (!IsValid(Rules) || !Rules->TryGetCostLimit(PlayerLevel, Validation.CostLimit) || Rules->WeaponSlots != 1 ||
		Rules->ArmorSlots != 3 || Rules->ActiveSkillSlots != 2 || Rules->PassiveSkillSlots != 1)
	{
		return Fail(ENF_BuildResult::InvalidRules, TEXT("코스트 상한과 슬롯 수(무기 1, 방어구 3, 액티브 2, 패시브 1)를 확인하세요."));
	}

	TMap<FName, const UNF_BuildItemDefinitionDataAsset*> Catalog;
	for (const UNF_BuildItemDefinitionDataAsset* Definition : Rules->Catalog)
	{
		if (!IsValid(Definition) || Definition->ItemId.IsNone() || Catalog.Contains(Definition->ItemId) ||
			Definition->Cost < 0 || Definition->Tier < 1 ||
			static_cast<uint8>(Definition->ItemType) > static_cast<uint8>(ENF_BuildItemType::PassiveSkill))
		{
			return Fail(ENF_BuildResult::InvalidRules, TEXT("카탈로그에 잘못된 데이터 또는 중복 ItemId가 있습니다."));
		}
		Catalog.Add(Definition->ItemId, Definition);
		if (Definition->ItemType == ENF_BuildItemType::Armor && Definition->GetDefaultSlotIndex() == INDEX_NONE)
		{
			return Fail(ENF_BuildResult::InvalidRules, TEXT("방어구 항목의 머리/상의/하의 부위를 지정해야 합니다."));
		}
	}

	if (Selections.Num() > Rules->WeaponSlots + Rules->ArmorSlots + Rules->ActiveSkillSlots + Rules->PassiveSkillSlots)
	{
		return Fail(ENF_BuildResult::InvalidSlot, TEXT("빌드 슬롯 수를 초과했습니다."));
	}

	TSet<FName> ItemIds;
	TSet<int32> OccupiedSlots;
	int64 TotalCost = 0;
	bool bHasWeapon = false;
	int32 LinkedWeaponCount = 0;
	for (const FNF_BuildSelection& Selection : Selections)
	{
		const UNF_BuildItemDefinitionDataAsset* const* Found = Catalog.Find(Selection.ItemId);
		if (!Found)
		{
			return Fail(ENF_BuildResult::UnknownItem, TEXT("상점 카탈로그에 없는 항목입니다."));
		}
		const UNF_BuildItemDefinitionDataAsset* Definition = *Found;
		if (ItemIds.Contains(Selection.ItemId))
		{
			return Fail(ENF_BuildResult::DuplicateItem, TEXT("동일한 항목을 중복 선택할 수 없습니다."));
		}
		ItemIds.Add(Selection.ItemId);

		if (!Definition->CanUseSlot(Selection.SlotIndex) ||
			Selection.SlotIndex >= Rules->GetSlotCount(Definition->ItemType))
		{
			return Fail(ENF_BuildResult::InvalidSlot, TEXT("아이템 종류 또는 방어구 부위와 맞지 않는 슬롯입니다."));
		}
		const int32 SlotKey = static_cast<int32>(Definition->ItemType) * 8 + Selection.SlotIndex;
		if (OccupiedSlots.Contains(SlotKey))
		{
			return Fail(ENF_BuildResult::InvalidSlot, TEXT("슬롯 범위가 잘못되었거나 이미 사용 중입니다."));
		}
		OccupiedSlots.Add(SlotKey);

		if (Definition->WeaponClass)
		{
			if (Definition->ItemType != ENF_BuildItemType::Weapon ||
				Definition->WeaponClass->HasAnyClassFlags(CLASS_Abstract))
			{
				return Fail(ENF_BuildResult::InvalidGrant, TEXT("무기 클래스는 Weapon 항목의 생성 가능한 클래스여야 합니다."));
			}
			if (Selection.SlotIndex != 0 || ++LinkedWeaponCount > 1)
			{
				return Fail(ENF_BuildResult::InvalidSlot, TEXT("실제 무기 장착은 무기 슬롯 1의 한 항목만 지원합니다."));
			}
		}

		if (Definition->AbilityLevel < 1 ||
			(Definition->AbilityClass && Definition->AbilityClass->HasAnyClassFlags(CLASS_Abstract)))
		{
			return Fail(ENF_BuildResult::InvalidGrant, TEXT("부여할 Ability의 클래스 또는 레벨이 잘못되었습니다."));
		}
		for (const TSubclassOf<UGameplayEffect>& EffectClass : Definition->EquippedEffects)
		{
			const UGameplayEffect* Effect = EffectClass ? EffectClass->GetDefaultObject<UGameplayEffect>() : nullptr;
			// UE 5.7's GetStackingType lacks a DLL export. Read the public legacy field for this engine version.
			PRAGMA_DISABLE_DEPRECATION_WARNINGS
			const bool bHasStacking = Effect && Effect->StackingType != EGameplayEffectStackingType::None;
			PRAGMA_ENABLE_DEPRECATION_WARNINGS
			if (!Effect || EffectClass->HasAnyClassFlags(CLASS_Abstract) ||
				Effect->DurationPolicy != EGameplayEffectDurationType::Infinite || bHasStacking ||
				Effect->Period.GetValueAtLevel(Definition->AbilityLevel) != 0.f || !Effect->Executions.IsEmpty() ||
				Effect->FindComponent<UGameplayEffectComponent>())
			{
				return Fail(ENF_BuildResult::InvalidGrant,
					TEXT("장착 GE는 Infinite, 비주기, 비중첩 Modifier만 "
						 "지원합니다. GE Component는 사용할 수 없습니다."));
			}
		}

		bHasWeapon |= Definition->ItemType == ENF_BuildItemType::Weapon;
		TotalCost += Definition->Cost;
		Validation.TotalCost = static_cast<int32>(FMath::Min<int64>(TotalCost, MAX_int32));
	}

	if (!bHasWeapon)
	{
		return Fail(ENF_BuildResult::RequiredWeaponMissing, TEXT("주무기를 하나 이상 선택해야 합니다."));
	}
	if (TotalCost > Validation.CostLimit)
	{
		return Fail(ENF_BuildResult::CostLimitExceeded, TEXT("레벨에서 제공하는 코스트 상한을 초과했습니다."));
	}
	Validation.Message = FText::FromString(TEXT("적용 가능한 빌드입니다."));
	return Validation;
}
