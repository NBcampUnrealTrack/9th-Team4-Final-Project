#include "GamePlay/BuildSystem/Core/NF_BuildMatchSubsystem.h"
#include "Core/NF_GameMode.h"
#include "Engine/World.h"
#include "GamePlay/BuildSystem/Data/NF_BuildRules.h"
#include "Core/NF_GameState.h"
#include "GamePlay/BuildSystem/GamePlay/BuildShop/NF_BuildShopZone.h"
#include "GameFramework/Pawn.h"
#include "GamePlay/BuildSystem/Player/NF_BuildComponent.h"
#include "Player/State/NF_PlayerState.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponent.h"
#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"
#include "GamePlay/Combat/Weapons/NF_WeaponDataAsset.h"
#include "GamePlay/Combat/Armor/NF_ArmorBase.h"
#include "GamePlay/Combat/Armor/NF_ArmorDataAsset.h"

bool UNF_BuildMatchSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return Super::ShouldCreateSubsystem(Outer) && World &&
		(World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UNF_BuildMatchSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (InWorld.GetNetMode() == NM_Client || RegisteredGameMode.IsValid())
	{
		return;
	}
	ANF_GameMode* GameMode = InWorld.GetAuthGameMode<ANF_GameMode>();
	if (!IsValid(GameMode))
	{
		return; // 로비 등 다른 GameMode에서는 기본 빌드 연동을 사용하지 않는다.
	}
	if (!GameMode->RegisterFallbackBuildHandler(
		FNF_ApplyFallbackBuild::CreateUObject(this, &ThisClass::HandleApplyFallbackBuild)))
	{
		UE_LOG(LogTemp, Error, TEXT("기본 빌드 콜백 등록 실패: Core API 또는 기존 등록을 확인하세요."));
		return;
	}
	RegisteredGameMode = GameMode;
}

bool UNF_BuildMatchSubsystem::HandleApplyFallbackBuild(ANF_PlayerState* PlayerState)
{
	if (!RegisteredGameMode.IsValid() || !RegisteredGameMode->HasAuthority() ||
		!IsValid(PlayerState) || !PlayerState->HasAuthority() || PlayerState->GetWorld() != GetWorld())
	{
		return false;
	}
	APawn* Pawn = PlayerState->GetPawn();
	if (!IsValid(Pawn) || Pawn->GetPlayerState() != PlayerState)
	{
		return false;
	}
	UNF_BuildComponent* Build = Pawn->FindComponentByClass<UNF_BuildComponent>();
	// 기본 적용은 세션·거리와 무관하게 해당 플레이어가 사용할 수 있는 팀 거점에서 규칙을 받는다.
	ANF_BuildShopZone* FallbackShop = nullptr;
	// 상점의 BP 접근 조건이 실행되는 동안 등록 목록이 변경될 수 있다.
	const TArray<TWeakObjectPtr<ANF_BuildShopZone>> RegisteredShops = Shops;
	for (const TWeakObjectPtr<ANF_BuildShopZone>& Entry : RegisteredShops)
	{
		ANF_BuildShopZone* Shop = Entry.Get();
		if (!IsValid(Shop) || !Shop->CanProvideFallbackBuild(Pawn)) continue;
		if (FallbackShop && FallbackShop->GetRules() != Shop->GetRules())
		{
			UE_LOG(LogTemp, Error, TEXT("기본 빌드 규칙 충돌: 같은 팀 거점의 Rules를 통일하세요."));
			return false;
		}
		FallbackShop = Shop;
	}
	const bool bSucceeded = IsValid(Build) && IsValid(FallbackShop) &&
		Build->InitializeAbilitySystem(PlayerState->GetAbilitySystemComponent()) &&
		Build->ApplyFallbackBuildForMatchStart(FallbackShop);
	if (!bSucceeded)
	{
		UE_LOG(LogTemp, Warning, TEXT("기본 빌드 적용 실패: %s. Pawn, ASC와 Rules를 확인하세요."),
		       *GetNameSafe(PlayerState));
	}
	return bSucceeded;
}

void UNF_BuildMatchSubsystem::Deinitialize()
{
	if (RegisteredGameMode.IsValid())
	{
		RegisteredGameMode->UnregisterFallbackBuildHandler(this);
	}
	RegisteredGameMode.Reset();
	Shops.Reset();
	Super::Deinitialize();
}

UNF_BuildItemDefinitionDataAsset* UNF_BuildMatchSubsystem::FindItemDefinition(
	const FNF_BuildQueryContext& Context, FName ItemId) const
{
	const UNF_BuildRules* Rules = Context.Rules;
	if (IsValid(Rules))
	{
		if (UNF_BuildItemDefinitionDataAsset* Definition = Rules->FindItemDefinition(ItemId)) { return Definition; }
	}
	for (const FNF_BuildEquippedItem& Item : Context.EquippedItems)
	{
		if (IsValid(Item.Definition) && Item.Definition->ItemId == ItemId) { return Item.Definition; }
	}
	return nullptr;
}

bool UNF_BuildMatchSubsystem::ResolveItemData(
	const FNF_BuildQueryContext& Context, FName ItemId, FNF_BuildItemData& OutData) const
{
	const UNF_BuildRules* Rules = Context.Rules;
	OutData = FNF_BuildItemData();
	if (IsValid(Rules) && Rules->FindItemDefinition(ItemId))
	{
		return Rules->ResolveItemData(ItemId, OutData);
	}
	for (const FNF_BuildEquippedItem& Item : Context.EquippedItems)
	{
		if (IsValid(Item.Definition) && Item.Definition->ItemId == ItemId)
		{
			OutData = Item.ItemData;
			return true;
		}
	}
	return false;
}

TArray<UNF_BuildItemDefinitionDataAsset*> UNF_BuildMatchSubsystem::GetCatalogItems(
	const FNF_BuildQueryContext& Context, ENF_ItemType ItemType,
	ENF_ArmorSlot ArmorPart) const
{
	const UNF_BuildRules* Rules = Context.Rules;
	TArray<UNF_BuildItemDefinitionDataAsset*> Result;

	if (!IsValid(Rules) || !Context.bInitialized)
	{
		return Result;
	}

	for (const FNF_BuildCatalogItem& Item : Rules->ReadCatalog())
	{
		if (!IsValid(Item.Definition) || Item.Data.Cost < 0 || Item.Data.ItemType != ItemType) { continue; }
		// 방어구는 지정한 부위만 표시한다. None이면 전체 부위.
		if (ItemType == ENF_ItemType::Armor &&
			ArmorPart != ENF_ArmorSlot::None &&
			Item.Data.ArmorSlot != ArmorPart) { continue; }
		Result.Add(Item.Definition);
	}

	return Result;
}

FNF_BuildValidation UNF_BuildMatchSubsystem::PreviewBuild(
	const FNF_BuildQueryContext& Context, const TArray<FNF_BuildSelection>& Selections) const
{
	if (!Context.bInitialized)
	{
		FNF_BuildValidation Validation;
		Validation.Result = ENF_BuildResult::InvalidRules;
		Validation.Message =
			FText::FromString(TEXT("플레이어의 빌드 조건이 아직 준비되지 않았습니다."));
		return Validation;
	}

	return ValidateBuild(Context.Rules, Context.CostLimit, Selections, Context.CurrentBuild, Context.PickedItemIds.Array());
}

FNF_BuildShopPreview UNF_BuildMatchSubsystem::GetShopPreview(const FNF_BuildQueryContext& Context) const
{
	FNF_BuildShopPreview Preview;
	const UNF_BuildRules* Rules = Context.Rules;
	const TArray<FNF_BuildCatalogItem> Items = IsValid(Rules) ? Rules->ReadCatalog() : TArray<FNF_BuildCatalogItem>{};
	const TArray<FNF_BuildSelection> FreeSelections = GetFreeSelections(Context.CurrentBuild, Context.PickedItemIds.Array());
	Preview.Validation = Context.bInitialized
		? UNF_BuildMatchSubsystem::ValidateCatalog(Rules, Items, Context.CostLimit, Context.DraftBuild, nullptr, &FreeSelections)
		: PreviewBuild(Context, Context.DraftBuild);

	TMap<FName, const FNF_BuildCatalogItem*> ById;
	for (const FNF_BuildCatalogItem& Item : Items)
	{
		if (IsValid(Item.Definition))
		{
			ById.FindOrAdd(Item.Definition->ItemId, &Item);
		}
	}

	TSet<FName> FreeIds;
	for (const FNF_BuildSelection& Selection : FreeSelections)
	{
		FreeIds.Add(Selection.ItemId);
	}

	int64 Total = 0;
	for (const FNF_BuildSelection& Selection : Context.DraftBuild)
	{
		FNF_BuildPreviewItem Row;
		if (const FNF_BuildCatalogItem* const* Item = ById.Find(Selection.ItemId))
		{
			Row = FNF_BuildPreviewItem(**Item);
		}
		else
		{
			// 현재 상점에 없는 기존 장착품은 복제된 장착 스냅샷으로 표시한다.
			Row.Definition = FindItemDefinition(Context, Selection.ItemId);
			ResolveItemData(Context, Selection.ItemId, Row.Data);
		}
		if (!IsValid(Row.Definition) || Row.Data.Cost < 0) continue;
		Row.SlotIndex = Selection.SlotIndex;
		if (!FreeIds.Contains(Selection.ItemId)) Total += Row.Data.Cost;
		Preview.Draft.Add(MoveTemp(Row));
	}
	// 검증이 중간에 실패해도 UI에는 전체 선택 금액을 표시한다.
	Preview.DisplayCost = static_cast<int32>(FMath::Min<int64>(Total, MAX_int32));

	if (!Context.bInitialized) return Preview;
	for (const FNF_BuildCatalogItem& Item : Items)
	{
		if (!IsValid(Item.Definition) || Item.Data.Cost < 0) continue;
		FNF_BuildPreviewItem& Row = Preview.Catalog.Emplace_GetRef(Item);
		if (Item.Data.ItemType == ENF_ItemType::ActiveSkill) continue;

		Row.SlotIndex = FNF_BuildItemSlots::GetDefaultSlotIndex(Item.Data);
		int64 CandidateCost = Total + Item.Data.Cost;
		for (const FNF_BuildPreviewItem& Existing : Preview.Draft)
		{
			const bool bReplaced = Existing.Data.ItemType == Item.Data.ItemType && Existing.SlotIndex == Row.SlotIndex;
			if ((bReplaced || Existing.Definition->ItemId == Item.Definition->ItemId) &&
				!FreeIds.Contains(Existing.Definition->ItemId))
			{
				CandidateCost -= Existing.Data.Cost;
			}
		}
		Row.CostAfterSelection = static_cast<int32>(FMath::Min<int64>(CandidateCost, MAX_int32));
	}
	return Preview;
}

UDataAsset* UNF_BuildMatchSubsystem::GetItemSource(const FNF_BuildQueryContext& Context, FName ItemId) const
{
	if (const UNF_BuildRules* Rules = Context.Rules)
	{
		if (Rules->FindItemDefinition(ItemId)) { return Rules->ItemSources.FindRef(ItemId); }
	}
	for (const FNF_BuildEquippedItem& Item : Context.EquippedItems)
	{
		if (IsValid(Item.Definition) && Item.Definition->ItemId == ItemId) { return Item.Source; }
	}
	return nullptr;
}

TArray<FNF_BuildSelection> UNF_BuildMatchSubsystem::GetFreeSelections(
	const TArray<FNF_BuildSelection>& CurrentBuild, const TArray<FName>& PaidItemIds) const
{
	TArray<FNF_BuildSelection> FreeSelections;
	for (const FNF_BuildSelection& Equipped : CurrentBuild)
	{
		if (!PaidItemIds.Contains(Equipped.ItemId))
		{
			FreeSelections.Add(Equipped);
		}
	}
	return FreeSelections;
}

void UNF_BuildMatchSubsystem::RegisterShop(ANF_BuildShopZone* Shop)
{
	if (!IsValid(Shop) || Shop->GetWorld() != GetWorld()) return;
	Shops.RemoveAll([](const TWeakObjectPtr<ANF_BuildShopZone>& Entry) { return !Entry.IsValid(); });
	Shops.AddUnique(Shop);
}

void UNF_BuildMatchSubsystem::UnregisterShop(ANF_BuildShopZone* Shop)
{
	Shops.RemoveAll([Shop](const TWeakObjectPtr<ANF_BuildShopZone>& Entry)
	{
		return !Entry.IsValid() || Entry.Get() == Shop;
	});
}

FNF_BuildValidation UNF_BuildMatchSubsystem::ValidateBuild(
	const UNF_BuildRules* Rules, int32 CostLimit, const TArray<FNF_BuildSelection>& Selections,
	const TArray<FNF_BuildSelection>& CurrentBuild, const TArray<FName>& PaidItemIds,
	TMap<FName, FNF_BuildItemData>* OutSelectedData) const
{
	const TArray<FNF_BuildSelection> FreeSelections = GetFreeSelections(CurrentBuild, PaidItemIds);
	return UNF_BuildMatchSubsystem::ValidateSelections(Rules, CostLimit, Selections, OutSelectedData, &FreeSelections);
}

bool UNF_BuildMatchSubsystem::CanApplyFallbackBuild(ANF_BuildShopZone* Shop, AActor* Player) const
{
	const ANF_GameState* GameState = GetWorld()->GetGameState<ANF_GameState>();
	return GetWorld()->GetNetMode() != NM_Client &&
		IsValid(Player) && Player->HasAuthority() && Player->GetWorld() == GetWorld() &&
		IsValid(Shop) && Shop->GetWorld() == GetWorld() && IsValid(Shop->GetRules()) &&
		IsValid(GameState) && GameState->GetMatchPhase() == ENF_MatchPhase::Build &&
		Shop->CanProvideFallbackBuild(Player);
}

ENF_BuildFallbackAction UNF_BuildMatchSubsystem::PrepareFallbackBuild(
	ANF_BuildShopZone* Shop, AActor* Player, int32 CostLimit,
	const TArray<FNF_BuildSelection>& CurrentBuild, bool bReady,
	TArray<FNF_BuildSelection>& OutSelections) const
{
	OutSelections.Reset();
	if (!CanApplyFallbackBuild(Shop, Player)) return ENF_BuildFallbackAction::Reject;
	const UNF_BuildRules* Rules = Shop->GetRules();
	// 매치 시작 정책은 기존 장착품을 포함한 전체 비용으로 준비 여부를 판단한다.
	if (bReady && !CurrentBuild.IsEmpty() &&
		UNF_BuildMatchSubsystem::ValidateSelections(Rules, CostLimit, CurrentBuild).Result == ENF_BuildResult::Success)
	{
		return ENF_BuildFallbackAction::KeepCurrent;
	}
	OutSelections = Rules->FallbackBuild;
	return ENF_BuildFallbackAction::Apply;
}

FNF_BuildValidation UNF_BuildMatchSubsystem::ValidateSelections(const UNF_BuildRules* Rules, int32 CostLimit,
                                                 const TArray<FNF_BuildSelection>& Selections,
                                                 TMap<FName, FNF_BuildItemData>* OutSelectedData,
                                                 const TArray<FNF_BuildSelection>* OwnedSelections)
{
	return ValidateCatalog(Rules, IsValid(Rules) ? Rules->ReadCatalog() : TArray<FNF_BuildCatalogItem>{},
	                       CostLimit, Selections, OutSelectedData, OwnedSelections);
}

FNF_BuildValidation UNF_BuildMatchSubsystem::ValidateCatalog(
	const UNF_BuildRules* Rules, const TArray<FNF_BuildCatalogItem>& Items, int32 CostLimit,
	const TArray<FNF_BuildSelection>& Selections, TMap<FName, FNF_BuildItemData>* OutSelectedData,
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

	if (!Rules->HasFixedSlotCounts())
	{
		return Fail(
			ENF_BuildResult::InvalidRules,
			TEXT("빌드 슬롯 수를 확인하세요."));
	}

	TMap<FName, const FNF_BuildItemData*> Catalog;
	// 임시: 원본 연결이 깨진 항목 하나 때문에 전체 빌드(시작 무기 포함)가 막히지 않도록,
	// 해석되지 않는 항목은 건너뛰고 해석되는 항목만 인식한다. 건너뛴 항목을 선택하면 InvalidRules로 거절한다.
	TSet<FName> SkippedItemIds;
	for (const FNF_BuildCatalogItem& Item : Items)
	{
		const UNF_BuildItemDefinitionDataAsset* Definition = Item.Definition;
		const FNF_BuildItemData& Data = Item.Data;
		const FName ItemId = IsValid(Definition) ? Definition->ItemId : NAME_None;
		// @Owner: 황준현 | 2026-11-05. 부위 미지정 방어구도 다른 설정 오류처럼 해당 항목만 제외한다.
		// 한 항목 때문에 상점 적용과 매치 시작 기본 빌드 전체가 막히지 않게 한다.
		if (ItemId.IsNone() || SkippedItemIds.Contains(ItemId) || Catalog.Contains(ItemId) ||
			Data.Cost < 0 ||
			static_cast<uint8>(Data.ItemType) > static_cast<uint8>(ENF_ItemType::PassiveSkill) ||
			(Data.ItemType == ENF_ItemType::Armor && FNF_BuildItemSlots::GetDefaultSlotIndex(Data) == INDEX_NONE))
		{
			if (!ItemId.IsNone())
			{
				// 중복 ID는 어느 원본이 맞는지 알 수 없으므로 먼저 등록된 항목도 함께 제외한다.
				Catalog.Remove(ItemId);
				SkippedItemIds.Add(ItemId);
			}
			UE_LOG(LogTemp, Warning,
			       TEXT("빌드 카탈로그 항목 제외: ItemId=%s. 원본 연결·코스트·종류·방어구 부위·중복 ID를 확인하세요."),
			       *ItemId.ToString());
			continue;
		}
		Catalog.Add(ItemId, &Data);
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
		const FNF_BuildItemData* Found = Catalog.FindRef(Selection.ItemId);

		if (SkippedItemIds.Contains(Selection.ItemId))
		{
			return Fail(ENF_BuildResult::InvalidRules,
			            TEXT("선택한 항목의 카탈로그 ID·원본 연결·코스트·종류를 확인하세요. 중복 ID 또는 미연결 원본은 사용할 수 없습니다."));
		}

		if (!Found)
		{
			return Fail(ENF_BuildResult::UnknownItem,TEXT("상점 카탈로그에 없는 항목입니다."));
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

		if (Data.ArmorClass && !UNF_BuildMatchSubsystem::IsArmorDefinitionCompatible(Data))
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
			OutSelectedData->Add(Selection.ItemId, *Catalog.FindChecked(Selection.ItemId));
		}
	}
	Validation.Message = FText::FromString(TEXT("적용 가능한 빌드입니다."));
	return Validation;
}

bool UNF_BuildMatchSubsystem::IsArmorDefinitionCompatible(const FNF_BuildItemData& Data)
{
	if (Data.ItemType != ENF_ItemType::Armor || !Data.ArmorClass ||
		Data.ArmorClass->HasAnyClassFlags(CLASS_Abstract))
	{
		return false;
	}
	return IsValid(Data.ArmorData) && Data.ArmorData->ItemType == ENF_ItemType::Armor &&
		Data.ArmorData->ArmorClass == Data.ArmorClass &&
		FNF_BuildItemSlots::GetDefaultSlotIndex(Data) != INDEX_NONE &&
		Data.ArmorData->ArmorSlot == Data.ArmorSlot;
}
