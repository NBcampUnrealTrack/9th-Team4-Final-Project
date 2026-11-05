#include "GamePlay/BuildSystem/Player/NF_BuildComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GamePlay/BuildSystem/Core/NF_BuildValidator.h"
#include "GamePlay/BuildSystem/Data/NF_BuildItemDefinitionDataAsset.h"
#include "GamePlay/BuildSystem/Data/NF_BuildRules.h"
#include "GamePlay/BuildSystem/Data/NF_BuildInitialSettings.h"
#include "GamePlay/Combat/Skill/Data/NF_SkillDataAsset.h"
#include "GAS/Tags/NFTag_Build.h"
#include "GamePlay/BuildSystem/GamePlay/BuildShop/NF_BuildShopZone.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffect.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"
#include "GamePlay/Combat/Weapons/NF_WeaponComponent.h"
#include "GamePlay/Combat/Weapons/NF_WeaponDataAsset.h"
#include "GamePlay/BuildSystem/Player/NF_BuildEquipmentAdapter.h"
#include "Net/UnrealNetwork.h"
#include "Player/Data/NF_PlayerActionDelegateSubsystem.h"
#include "Core/NF_GameState.h"

UNF_BuildComponent::UNF_BuildComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickInterval = 0.2f;
	SetIsReplicatedByDefault(true);
}

void UNF_BuildComponent::BeginPlay()
{
	Super::BeginPlay();
	SetComponentTickEnabled(GetOwner()->HasAuthority());
	if (GetOwner()->HasAuthority())
	{
		InitializeDefaultBuildEntitlements();
		UpdateShopsInRange();
	}
}

void UNF_BuildComponent::InitializeDefaultBuildEntitlements()
{
	// 외부 시스템이 먼저 전달한 개인별 조건을 기본값으로 덮어쓰지 않는다.
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority() ||
		bEntitlementsInitialized || (!IsValid(InitialSettings) && !bInitializeEntitlementsFromDefaults))
	{
		return;
	}

	const int32 StartingCostLimit = IsValid(InitialSettings) ? InitialSettings->InitialCostLimit : InitialCostLimit;
	if (!UpdateBuildCostLimit(StartingCostLimit))
	{
		UE_LOG(LogTemp, Warning,
		       TEXT("빌드 초기 조건 적용 실패: %s. 초기 설정 DA의 코스트를 확인하세요."),
		       *GetNameSafe(GetOwner()));
	}
}

TArray<ANF_BuildShopZone*> UNF_BuildComponent::GetShopsInRange() const
{
	TArray<ANF_BuildShopZone*> Result;
	for (ANF_BuildShopZone* Shop : ShopsInRange)
	{
		if (IsValid(Shop))
		{
			Result.Add(Shop);
		}
	}
	return Result;
}

void UNF_BuildComponent::UpdateShopsInRange()
{
	TArray<TObjectPtr<ANF_BuildShopZone>> NewShops;
	for (TActorIterator<ANF_BuildShopZone> Iterator(GetWorld()); Iterator; ++Iterator)
	{
		if (Iterator->IsActorInRange(GetOwner()))
		{
			NewShops.Add(*Iterator);
		}
	}
	if (ShopsInRange.Num() == NewShops.Num() &&
		NewShops.ContainsByPredicate([this](const TObjectPtr<ANF_BuildShopZone>& Shop)
		{
			return !ShopsInRange.Contains(Shop);
		}) == false)
	{
		return;
	}
	ShopsInRange = MoveTemp(NewShops);
	GetOwner()->ForceNetUpdate();
	OnRep_ShopsInRange();
}

void UNF_BuildComponent::OnRep_ShopsInRange()
{
	// 알림 중 실행되는 외부 콜백이 영향을 주지 않도록 이전/현재 목록을 먼저 확정한다.
	const TArray<TWeakObjectPtr<ANF_BuildShopZone>> Previous = NotifiedShopsInRange;
	TArray<TWeakObjectPtr<ANF_BuildShopZone>> Current;
	for (ANF_BuildShopZone* Shop : ShopsInRange)
	{
		if (IsValid(Shop))
		{
			Current.Add(Shop);
		}
	}
	NotifiedShopsInRange = Current;
	for (const TWeakObjectPtr<ANF_BuildShopZone>& Shop : Previous)
	{
		if (!Current.Contains(Shop))
		{
			// 상점이 파괴된 경우 참조는 nullptr일 수 있다.
			OnBuildShopRangeChanged.Broadcast(Shop.Get(), ENF_BuildShopRangeState::Outside);
		}
	}
	for (const TWeakObjectPtr<ANF_BuildShopZone>& Shop : Current)
	{
		if (!Previous.Contains(Shop))
		{
			OnBuildShopRangeChanged.Broadcast(Shop.Get(), ENF_BuildShopRangeState::Inside);
		}
	}
}

UAbilitySystemComponent* UNF_BuildComponent::ResolveAbilitySystem() const
{
	if (IsValid(AbilitySystem))
	{
		return AbilitySystem;
	}
	const IAbilitySystemInterface* Provider = Cast<IAbilitySystemInterface>(GetOwner());
	return Provider ? Provider->GetAbilitySystemComponent() : nullptr;
}

bool UNF_BuildComponent::BeginBuildSession(ANF_BuildShopZone* Shop)
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!GetOwner()->HasAuthority() || bApplyingBuild || !Pawn || !Cast<APlayerController>(Pawn->GetController()) ||
		!IsValid(Shop) || Shop->GetWorld() != GetWorld() || !Shop->CanBuild(Pawn) ||
		!InitializeAbilitySystem(ResolveAbilitySystem()) ||
		AbilitySystem->HasMatchingGameplayTag(NFTag_Build::State_Build_Locked))
	{
		return false;
	}
	if (CurrentShop == Shop && SessionController.Get() == Pawn->GetController())
	{
		return true;
	}
	CurrentShop = Shop;
	SessionController = Cast<APlayerController>(Pawn->GetController());
	SetComponentTickEnabled(true);
	GetOwner()->ForceNetUpdate();
	OnRep_CurrentShop();
	return true;
}

void UNF_BuildComponent::CloseBuildSession()
{
	if (!GetOwner()->HasAuthority())
	{
		ServerCloseBuildSession();
		return;
	}
	if (!CurrentShop && !SessionController.IsValid())
	{
		return;
	}
	CurrentShop = nullptr;
	SessionController.Reset();
	// 닫은 뒤에도 범위 진입·이탈 검사는 계속한다.
	GetOwner()->ForceNetUpdate();
	OnRep_CurrentShop();
}

void UNF_BuildComponent::ServerCloseBuildSession_Implementation()
{
	CloseBuildSession();
}

void UNF_BuildComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
	Super::TickComponent(DeltaTime, TickType, TickFunction);
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	UpdateShopsInRange();
	APawn* Pawn = Cast<APawn>(GetOwner());
	if ((CurrentShop || SessionController.IsValid()) && (!IsValid(CurrentShop) || !CurrentShop->CanBuild(GetOwner()) ||
		!Pawn || !SessionController.IsValid() || Pawn->GetController() != SessionController.Get() || !IsValid(
			AbilitySystem) ||
		AbilitySystem->GetAvatarActor() != GetOwner() ||
		AbilitySystem->HasMatchingGameplayTag(NFTag_Build::State_Build_Locked)))
	{
		CloseBuildSession();
	}
}

UNF_BuildItemDefinitionDataAsset* UNF_BuildComponent::FindItemDefinition(FName ItemId) const
{
	const UNF_BuildRules* Rules = GetRules();
	if (IsValid(Rules))
	{
		if (UNF_BuildItemDefinitionDataAsset* Definition = Rules->FindItemDefinition(ItemId)) { return Definition; }
	}
	for (const FNF_BuildEquippedItem& Item : EquippedItems)
	{
		if (IsValid(Item.Definition) && Item.Definition->ItemId == ItemId) { return Item.Definition; }
	}
	return nullptr;
}

bool UNF_BuildComponent::ResolveItemData(FName ItemId, FNF_BuildItemData& OutData) const
{
	const UNF_BuildRules* Rules = GetRules();
	OutData = FNF_BuildItemData();
	if (IsValid(Rules) && Rules->FindItemDefinition(ItemId))
	{
		return Rules->ResolveItemData(ItemId, OutData);
	}
	for (const FNF_BuildEquippedItem& Item : EquippedItems)
	{
		if (IsValid(Item.Definition) && Item.Definition->ItemId == ItemId)
		{
			OutData = Item.ItemData;
			return true;
		}
	}
	return false;
}

TArray<UNF_BuildItemDefinitionDataAsset*> UNF_BuildComponent::GetCatalogItems(
	ENF_ItemType ItemType,
	ENF_ArmorSlot ArmorPart) const
{
	const UNF_BuildRules* Rules = GetRules();
	TArray<UNF_BuildItemDefinitionDataAsset*> Result;

	if (!IsValid(Rules) || !bEntitlementsInitialized)
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

UNF_BuildItemDefinitionDataAsset* UNF_BuildComponent::GetSlotItem(
	ENF_ItemType ItemType, int32 SlotIndex, bool bUseDraft) const
{
	const TArray<FNF_BuildSelection>& Selections = bUseDraft ? DraftBuild : CurrentBuild;
	for (const FNF_BuildSelection& Selection : Selections)
	{
		UNF_BuildItemDefinitionDataAsset* Definition = FindItemDefinition(Selection.ItemId);
		FNF_BuildItemData Data;
		if (Definition && ResolveItemData(Selection.ItemId, Data) && Data.ItemType == ItemType &&
			Selection.SlotIndex == SlotIndex && FNF_BuildItemSlots::CanUseSlot(Data, SlotIndex))
		{
			return Definition;
		}
	}
	return nullptr;
}

bool UNF_BuildComponent::CanEditDraft() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	return Pawn && Pawn->IsLocallyControlled() && IsValid(CurrentShop) && !bAwaitingBuildResult;
}

bool UNF_BuildComponent::SelectDraftItem(FName ItemId)
{
	const UNF_BuildRules* Rules = GetRules();
	FNF_BuildItemData Data;
	if (!CanEditDraft() || !IsValid(Rules) || !ResolveItemData(ItemId, Data) || Rules->GetSlotCount(Data.ItemType) < 1)
	{
		return false;
	}
	PendingActiveSkill = Data.ItemType == ENF_ItemType::ActiveSkill ? ItemId : NAME_None;
	if (PendingActiveSkill.IsNone())
	{
		const int32 TargetSlotIndex = FNF_BuildItemSlots::GetDefaultSlotIndex(Data);
		if (!FNF_BuildItemSlots::CanUseSlot(Data, TargetSlotIndex) || TargetSlotIndex >= Rules->GetSlotCount(
			Data.ItemType))
		{
			return false;
		}
		DraftBuild.RemoveAll([this, Data, TargetSlotIndex](const FNF_BuildSelection& Selection)
		{
			FNF_BuildItemData Existing;
			return ResolveItemData(Selection.ItemId, Existing) && Existing.ItemType == Data.ItemType && Selection.
				SlotIndex == TargetSlotIndex;
		});
		FNF_BuildSelection Selection;
		Selection.ItemId = ItemId;
		Selection.SlotIndex = TargetSlotIndex;
		DraftBuild.Add(Selection);
		PickedItemIds.Add(ItemId);
		bDraftDirty = true;
	}
	OnDraftChanged.Broadcast();
	return true;
}

bool UNF_BuildComponent::AssignSelectedActiveSkill(int32 SlotIndex)
{
	const UNF_BuildRules* Rules = GetRules();
	FNF_BuildItemData Data;
	if (!CanEditDraft() || !IsValid(Rules) || !ResolveItemData(PendingActiveSkill, Data) || Data.ItemType !=
		ENF_ItemType::ActiveSkill ||
		SlotIndex < 0 || SlotIndex > 1 || SlotIndex >= Rules->ActiveSkillSlots)
	{
		return false;
	}
	DraftBuild.RemoveAll([this, SlotIndex](const FNF_BuildSelection& Selection)
	{
		FNF_BuildItemData Existing;
		const bool bResolved = ResolveItemData(Selection.ItemId, Existing);
		return Selection.ItemId == PendingActiveSkill ||
			(bResolved && Existing.ItemType == ENF_ItemType::ActiveSkill && Selection.SlotIndex == SlotIndex);
	});
	FNF_BuildSelection Selection;
	Selection.ItemId = PendingActiveSkill;
	Selection.SlotIndex = SlotIndex;
	DraftBuild.Add(Selection);
	PickedItemIds.Add(PendingActiveSkill);
	PendingActiveSkill = NAME_None;
	bDraftDirty = true;
	OnDraftChanged.Broadcast();
	return true;
}

bool UNF_BuildComponent::RemoveDraftItem(FName ItemId)
{
	if (!CanEditDraft())
	{
		return false;
	}
	const bool bRemoved = DraftBuild.RemoveAll([ItemId](const FNF_BuildSelection& Selection)
	{
		return Selection.ItemId == ItemId;
	}) > 0;
	if (bRemoved)
	{
		PickedItemIds.Remove(ItemId);
		bDraftDirty = true;
		OnDraftChanged.Broadcast();
	}
	return bRemoved;
}

bool UNF_BuildComponent::RevertDraftItem(FName ItemId)
{
	if (!CanEditDraft())
	{
		return false;
	}
	// 칸 선택을 기다리는 액티브 스킬은 대기만 취소한다.
	if (PendingActiveSkill == ItemId)
	{
		PendingActiveSkill = NAME_None;
		OnDraftChanged.Broadcast();
		return true;
	}
	const bool bRemoved = DraftBuild.RemoveAll([ItemId](const FNF_BuildSelection& Selection)
	{
		return Selection.ItemId == ItemId;
	}) > 0;
	if (!bRemoved)
	{
		return false;
	}
	PickedItemIds.Remove(ItemId);
	bDraftDirty = true;
	OnDraftChanged.Broadcast();
	return true;
}

void UNF_BuildComponent::ApplyDraftBuild()
{
	if (CanEditDraft() && PendingActiveSkill.IsNone() && PreviewBuild(DraftBuild).Result == ENF_BuildResult::Success)
	{
		bAwaitingBuildResult = true;
		OnDraftChanged.Broadcast();
		SendApplyBuild(DraftBuild, CurrentShop, PickedItemIds.Array());
	}
}


void UNF_BuildComponent::OnRep_CurrentShop()
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (Pawn && Pawn->IsLocallyControlled())
	{
		// 상점을 열 때마다 빈 빌드에서 시작한다. 적용하면 이번에 고른 항목만 남고 이전 빌드는 모두 빠진다.
		// 적용하지 않고 닫으면 CurrentBuild는 그대로다.
		DraftBuild.Reset();
		PendingActiveSkill = NAME_None;
		PickedItemIds.Reset();
		bDraftDirty = false;

		OnDraftChanged.Broadcast();
	}

	const ENF_BuildShopUIState SessionState =
		IsValid(CurrentShop)
			? ENF_BuildShopUIState::BuildOpened
			: ENF_BuildShopUIState::BuildClosed;

	OnBuildShopStateChanged.Broadcast(SessionState);

	// 상점 사용 상태가 다른 플레이어에게 적용되지 않도록 대상 컨트롤러를 함께 전달한다.
	APlayerController* SendController = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (IsValid(SendController))
	{
		NF_BROADCAST_TO_PLAYER_DELEGATE_TwoParams(OnInteractBuildShop, SendController, SessionState);
	}
}

void UNF_BuildComponent::OnRep_BuildConfiguration()
{
	OnDraftChanged.Broadcast();
}

bool UNF_BuildComponent::InitializeAbilitySystem(UAbilitySystemComponent* InAbilitySystem)
{
	if (!GetOwner()->HasAuthority() || bApplyingBuild || !IsValid(InAbilitySystem) ||
		InAbilitySystem->GetAvatarActor() != GetOwner() ||
		(!AppliedItems.IsEmpty() && AbilitySystem != InAbilitySystem))
	{
		return false;
	}
	AbilitySystem = InAbilitySystem;
	return true;
}

bool UNF_BuildComponent::SetPlayerLevel(int32 NewLevel)
{
	// @Owner: 황준현 | 2026-10-20. 진행 시스템의 레벨 결정을 빌드 유효성으로 거절하지 않는다.
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority() || bApplyingBuild || NewLevel < 1)
	{
		return false;
	}
	PlayerLevel = NewLevel;
	GetOwner()->ForceNetUpdate();
	OnDraftChanged.Broadcast();
	return true;
}

FNF_BuildValidation UNF_BuildComponent::PreviewBuild(
	const TArray<FNF_BuildSelection>& Selections) const
{
	if (!bEntitlementsInitialized)
	{
		FNF_BuildValidation Validation;
		Validation.Result = ENF_BuildResult::InvalidRules;
		Validation.Message =
			FText::FromString(TEXT("플레이어의 빌드 조건이 아직 준비되지 않았습니다."));
		return Validation;
	}

	const TArray<FNF_BuildSelection> FreeSelections = GetFreeSelections(PickedItemIds.Array());
	return FNF_BuildValidator::Validate(
		GetRules(),
		CostLimit,
		Selections,
		nullptr,
		&FreeSelections);
}

FNF_BuildShopPreview UNF_BuildComponent::GetShopPreview() const
{
	FNF_BuildShopPreview Preview;
	const UNF_BuildRules* Rules = GetRules();
	const TArray<FNF_BuildCatalogItem> Items = IsValid(Rules) ? Rules->ReadCatalog() : TArray<FNF_BuildCatalogItem>{};
	const TArray<FNF_BuildSelection> FreeSelections = GetFreeSelections(PickedItemIds.Array());
	Preview.Validation = bEntitlementsInitialized
		? FNF_BuildValidator::ValidateCatalog(Rules, Items, CostLimit, DraftBuild, nullptr, &FreeSelections)
		: PreviewBuild(DraftBuild);

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
	for (const FNF_BuildSelection& Selection : DraftBuild)
	{
		FNF_BuildPreviewItem Row;
		if (const FNF_BuildCatalogItem* const* Item = ById.Find(Selection.ItemId))
		{
			Row = FNF_BuildPreviewItem(**Item);
		}
		else
		{
			// 현재 상점에 없는 기존 장착품은 복제된 장착 스냅샷으로 표시한다.
			Row.Definition = FindItemDefinition(Selection.ItemId);
			ResolveItemData(Selection.ItemId, Row.Data);
		}
		if (!IsValid(Row.Definition) || Row.Data.Cost < 0) continue;
		Row.SlotIndex = Selection.SlotIndex;
		if (!FreeIds.Contains(Selection.ItemId)) Total += Row.Data.Cost;
		Preview.Draft.Add(MoveTemp(Row));
	}
	// 검증이 중간에 실패해도 UI에는 전체 선택 금액을 표시한다.
	Preview.DisplayCost = static_cast<int32>(FMath::Min<int64>(Total, MAX_int32));

	if (!bEntitlementsInitialized) return Preview;
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

TArray<FNF_BuildSelection> UNF_BuildComponent::GetFreeSelections(const TArray<FName>& PaidItemIds) const
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

void UNF_BuildComponent::RequestApplyBuild(const TArray<FNF_BuildSelection>& Selections, ANF_BuildShopZone* ShopZone)
{
	SendApplyBuild(Selections, ShopZone, {});
}

void UNF_BuildComponent::SendApplyBuild(const TArray<FNF_BuildSelection>& Selections, ANF_BuildShopZone* ShopZone,
                                        const TArray<FName>& PaidItemIds)
{
	if (GetOwner()->HasAuthority())
	{
		CompleteLocalBuildRequest(ApplyBuildOnServer(Selections, ShopZone, PaidItemIds));
	}
	else
	{
		ServerApplyBuild(Selections, ShopZone, PaidItemIds);
	}
}

void UNF_BuildComponent::ServerApplyBuild_Implementation(
	const TArray<FNF_BuildSelection>& Selections, ANF_BuildShopZone* ShopZone, const TArray<FName>& PaidItemIds)
{
	// 클라이언트가 보낸 목록은 장착 항목의 무료 처리를 없애기만 하므로 코스트를 줄일 수 없다.
	ClientBuildRequestCompleted(ApplyBuildOnServer(Selections, ShopZone, PaidItemIds));
}

void UNF_BuildComponent::ClientBuildRequestCompleted_Implementation(const FNF_BuildValidation& Validation)
{
	// The replicated CurrentBuild is the source of truth; RPC and property arrival order may differ.
	CompleteLocalBuildRequest(Validation);
}

void UNF_BuildComponent::CompleteLocalBuildRequest(const FNF_BuildValidation& Validation)
{
	bAwaitingBuildResult = false;
	if (Validation.Result == ENF_BuildResult::Success)
	{
		bDraftDirty = false;
	}
	OnDraftChanged.Broadcast();
	OnBuildRequestCompleted.Broadcast(Validation);
}


FNF_BuildValidation UNF_BuildComponent::ApplyBuildOnServer(
	const TArray<FNF_BuildSelection>& Selections,
	ANF_BuildShopZone* ShopZone,
	const TArray<FName>& PaidItemIds)
{
	FNF_BuildValidation Validation;

	auto Fail = [&Validation](ENF_BuildResult Result, const TCHAR* Message)
	{
		Validation.Result = Result;
		Validation.Message = FText::FromString(Message);
		return Validation;
	};

	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority())
	{
		return Fail(
			ENF_BuildResult::NotAuthority,
			TEXT("빌드 적용은 서버에서만 가능합니다."));
	}

	const APawn* PlayerPawn = Cast<APawn>(GetOwner());

	if (!IsValid(ShopZone) ||
		CurrentShop != ShopZone ||
		!SessionController.IsValid() ||
		!PlayerPawn ||
		PlayerPawn->GetController() != SessionController.Get() ||
		ShopZone->GetWorld() != GetWorld() ||
		!ShopZone->CanBuild(GetOwner()))
	{
		return Fail(
			ENF_BuildResult::NotInBuildZone,
			TEXT("상점과 먼저 상호작용하고 사용 조건을 만족해야 합니다."));
	}

	return CommitBuildOnServer(Selections, ShopZone->GetRules(), PaidItemIds);
}

bool UNF_BuildComponent::ApplyFallbackBuildForMatchStart(ANF_BuildShopZone* Shop)
{
	const UNF_BuildRules* Rules = IsValid(Shop) ? Shop->GetRules() : nullptr;
	const ANF_GameState* GameState = GetWorld() ? GetWorld()->GetGameState<ANF_GameState>() : nullptr;
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority() || bApplyingBuild ||
		!IsValid(GameState) || GameState->GetMatchPhase() != ENF_MatchPhase::Build || !IsValid(Rules) ||
		Shop->GetWorld() != GetWorld() || !Shop->CanProvideFallbackBuild(GetOwner()))
	{
		return false;
	}
	if (!InitializeAbilitySystem(ResolveAbilitySystem()))
	{
		return false;
	}
	// @Owner: 황준현 | 2026-11-05. 외부 진행 시스템이 아직 코스트를 주지 않았으면 초기 설정 DA로 먼저 채운다.
	// 그래도 준비되지 않으면 무기 없이 출격하지 않도록 원인을 남긴다.
	if (!bEntitlementsInitialized)
	{
		InitializeDefaultBuildEntitlements();
	}
	if (!bEntitlementsInitialized)
	{
		UE_LOG(LogTemp, Error,
		       TEXT("기본 빌드 적용 실패: %s의 코스트 상한이 초기화되지 않았습니다. 빌드 컴포넌트의 InitialSettings DA를 지정하세요."),
		       *GetNameSafe(GetOwner()));
		return false;
	}
	// 이미 유효하게 준비된 빌드는 반복 요청으로 덮어쓰지 않는다. 장착 항목도 무료로 보지 않고 전체 코스트로 판정한다.
	if (!CurrentBuild.IsEmpty() &&
		FNF_BuildValidator::Validate(Rules, CostLimit, CurrentBuild).Result == ENF_BuildResult::Success &&
		AbilitySystem->HasMatchingGameplayTag(NFTag_Build::State_Build_Ready))
	{
		return true;
	}
	// 적용 중 콜백으로 원본 구성이 바뀌지 않도록 복사한다.
	const TArray<FNF_BuildSelection> Selections = Rules->FallbackBuild;
	// 기본 빌드는 모든 항목을 유료로 계산한다. 기존 장착 항목이 겹쳐도 상한 검사를 건너뛰지 않는다.
	TArray<FName> PaidItemIds;
	for (const FNF_BuildSelection& Selection : Selections)
	{
		PaidItemIds.Add(Selection.ItemId);
	}
	const FNF_BuildValidation Validation = CommitBuildOnServer(Selections, Rules, PaidItemIds);
	const bool bSucceeded = Validation.Result == ENF_BuildResult::Success &&
		IsValid(AbilitySystem) && AbilitySystem->HasMatchingGameplayTag(NFTag_Build::State_Build_Ready);
	if (bSucceeded)
	{
		CloseBuildSession();
	}
	else
	{
		// 잠금 상태 등 정상적인 거절도 이 경로를 지나므로 경고로 남긴다.
		UE_LOG(LogTemp, Warning,
		       TEXT("기본 빌드 적용 실패: %s, Rules=%s, 결과=%d, 코스트 %d / %d, 사유: %s"),
		       *GetNameSafe(GetOwner()), *GetNameSafe(Rules), static_cast<int32>(Validation.Result),
		       Validation.TotalCost, CostLimit, *Validation.Message.ToString());
	}
	return bSucceeded;
}

void UNF_BuildComponent::MarkBuildReady()
{
	if (!AbilitySystem->HasMatchingGameplayTag(NFTag_Build::State_Build_Ready))
	{
		// UE 5.7 API로 서버 태그와 소유 클라이언트의 카운트를 함께 갱신한다.
		ReadyAbilitySystem = AbilitySystem;
		bOwnsReadyTag = true;
		AbilitySystem->AddLooseGameplayTag(
			NFTag_Build::State_Build_Ready, 1, EGameplayTagReplicationState::CountToOwner);
	}
}

FNF_BuildValidation UNF_BuildComponent::CommitBuildOnServer(
	const TArray<FNF_BuildSelection>& Selections, const UNF_BuildRules* Rules, const TArray<FName>& PaidItemIds)
{
	FNF_BuildValidation Validation;
	auto Fail = [&Validation](ENF_BuildResult Result, const TCHAR* Message)
	{
		Validation.Result = Result;
		Validation.Message = FText::FromString(Message);
		return Validation;
	};

	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority())
	{
		return Fail(
			ENF_BuildResult::NotAuthority,
			TEXT("빌드 적용은 서버에서만 가능합니다."));
	}

	if (bApplyingBuild)
	{
		return Fail(ENF_BuildResult::BuildBlocked, TEXT("빌드 적용이 진행 중입니다."));
	}
	TGuardValue<bool> ApplyGuard(bApplyingBuild, true);

	if (!IsValid(AbilitySystem) ||
		AbilitySystem->GetAvatarActor() != GetOwner())
	{
		return Fail(
			ENF_BuildResult::MissingAbilitySystem,
			TEXT("플레이어 ASC의 ActorInfo와 빌드 컴포넌트를 연결해야 합니다."));
	}

	if (AbilitySystem->HasMatchingGameplayTag(
		NFTag_Build::State_Build_Locked))
	{
		return Fail(
			ENF_BuildResult::BuildBlocked,
			TEXT("현재 상태에서는 빌드를 교체할 수 없습니다."));
	}

	// 상점 검사는 이 함수에 넣지 않는다.
	TMap<FName, FNF_BuildItemData> SelectedData;
	if (!bEntitlementsInitialized)
	{
		return PreviewBuild(Selections);
	}
	const TArray<FNF_BuildSelection> FreeSelections = GetFreeSelections(PaidItemIds);
	Validation = FNF_BuildValidator::Validate(Rules, CostLimit, Selections, &SelectedData,
	                                          &FreeSelections);
	if (Validation.Result != ENF_BuildResult::Success)
	{
		return Validation;
	}

	// ItemId는 상점 간에도 같은 항목을 뜻해야 한다. 서로 다른 원본에 기존 GAS 핸들을 재사용하지 않는다.
	for (const FNF_BuildSelection& Selection : Selections)
	{
		for (const FNF_BuildEquippedItem& Equipped : EquippedItems)
		{
			if (IsValid(Equipped.Definition) && Equipped.Definition->ItemId == Selection.ItemId &&
				(Equipped.Definition != Rules->FindItemDefinition(Selection.ItemId) ||
					Equipped.Source != Rules->ItemSources.FindRef(Selection.ItemId)))
			{
				return Fail(ENF_BuildResult::InvalidRules, TEXT("상점 간 동일 ItemId의 정의와 원본이 일치해야 합니다."));
			}
		}
	}

	TMap<FName, FNF_BuildGrantHandles> StagedItems;
	TSet<FName> SelectedIds;
	for (const FNF_BuildSelection& Selection : Selections)
	{
		SelectedIds.Add(Selection.ItemId);
		if (AppliedItems.Contains(Selection.ItemId))
		{
			// Keep handles to preserve running abilities and avoid resetting unchanged passives.
			continue;
		}
		UNF_BuildItemDefinitionDataAsset* Definition = nullptr;
		for (UNF_BuildItemDefinitionDataAsset* Candidate : Rules->Catalog)
		{
			if (IsValid(Candidate) && Candidate->ItemId == Selection.ItemId)
			{
				Definition = Candidate;
				break;
			}
		}
		FNF_BuildGrantHandles Handles;
		if (!GrantItem(Definition, Rules->ItemSources.FindRef(Selection.ItemId).Get(), 
			SelectedData.FindChecked(Selection.ItemId), Selection.SlotIndex, Handles))
		{
			RevokeItem(Handles);
			for (const TPair<FName, FNF_BuildGrantHandles>& Staged : StagedItems)
			{
				RevokeItem(Staged.Value);
			}
			return Fail(ENF_BuildResult::GrantFailed, TEXT("GAS 부여에 실패했습니다. 기존 빌드를 유지합니다."));
		}
		StagedItems.Add(Selection.ItemId, MoveTemp(Handles));
	}

	// 장비 전체 적용이 성공하기 전에는 기존 GAS 부여를 회수하지 않는다.
	if (!ApplyEquipmentSelection(Selections, SelectedData))
	{
		for (const TPair<FName, FNF_BuildGrantHandles>& Staged : StagedItems)
		{
			RevokeItem(Staged.Value);
		}
		return Fail(ENF_BuildResult::GrantFailed, TEXT("장비 적용 실패: 원본 DA·장비 컴포넌트·무기 DA 수신 구현을 확인하세요."));
	}

	for (auto Iterator = AppliedItems.CreateIterator(); Iterator; ++Iterator)
	{
		if (!SelectedIds.Contains(Iterator.Key()))
		{
			RevokeItem(Iterator.Value());
			Iterator.RemoveCurrent();
		}
	}
	for (TPair<FName, FNF_BuildGrantHandles>& Staged : StagedItems)
	{
		AppliedItems.Add(Staged.Key, MoveTemp(Staged.Value));
	}
	for (const FNF_BuildSelection& Selection : Selections)
	{
		const FNF_BuildGrantHandles& Handles = AppliedItems.FindChecked(Selection.ItemId);
		if (Handles.ItemData.ItemType == ENF_ItemType::ActiveSkill)
		{
			if (FGameplayAbilitySpec* Spec = AbilitySystem->FindAbilitySpecFromHandle(Handles.AbilityHandle))
			{
				// 플레이어 입력과 동일하게 액티브 슬롯 0/1을 InputID로 사용하며 기존 Spec은 유지한다.
				Spec->InputID = Selection.SlotIndex;
				AbilitySystem->MarkAbilitySpecDirty(*Spec);
			}
		}
	}

	EquippedItems.Reset();
	for (const FNF_BuildSelection& Selection : Selections)
	{
		FNF_BuildEquippedItem& Item = EquippedItems.AddDefaulted_GetRef();
		Item.Definition = Rules->FindItemDefinition(Selection.ItemId);
		Item.Source = Rules->ItemSources.FindRef(Selection.ItemId);
		Item.ItemData = AppliedItems.FindChecked(Selection.ItemId).ItemData;
	}
	CurrentBuild = Selections;
	GetOwner()->ForceNetUpdate();
	MarkBuildReady();
	OnRep_CurrentBuild();
	Validation.Message = FText::FromString(TEXT("빌드를 적용했습니다."));
	return Validation;
}

// @Owner: 황준현 | 2026-10-15. 빌드는 무기 선택과 생성을, 무기 컴포넌트는 장착과 전투를 담당한다.
bool UNF_BuildComponent::ApplyWeaponSelection(const TArray<FNF_BuildSelection>& Selections,
                                              const TMap<FName, FNF_BuildItemData>& SelectedData)
{
	const FNF_BuildItemData* SelectedWeapon = nullptr;
	FName SelectedWeaponId;
	for (const FNF_BuildSelection& Selection : Selections)
	{
		const FNF_BuildItemData* Definition = SelectedData.Find(Selection.ItemId);
		if (Definition && Definition->ItemType == ENF_ItemType::Weapon && Definition->WeaponClass)
		{
			// 팀 공용 무기 컴포넌트는 무기 한 개의 장착만 지원한다.
			if (SelectedWeapon || Selection.SlotIndex != 0)
			{
				return false;
			}
			SelectedWeapon = Definition;
			SelectedWeaponId = Selection.ItemId;
		}
	}
	if (!SelectedWeapon)
	{
		ReleaseBuildWeapon();
		return true;
	}
	UNF_WeaponComponent* Weapons = GetOwner()->FindComponentByClass<UNF_WeaponComponent>();
	if (!Weapons)
	{
		return false;
	}
	if (BuildWeapon.IsValid() && BuildWeaponItemId == SelectedWeaponId &&
		Weapons->GetEquippedWeapon() == BuildWeapon.Get() &&
		BuildWeapon->GetClass() == SelectedWeapon->WeaponClass.Get() &&
		BuildWeapon->GetWeaponData() == SelectedWeapon->WeaponData.Get())
	{
		return true; // 동일한 무기를 유지하여 탄약과 재장전 상태를 보존한다.
	}
	FActorSpawnParameters Parameters;
	Parameters.Owner = GetOwner();
	Parameters.Instigator = Cast<APawn>(GetOwner());
	ANF_WeaponBase* NewWeapon = GetWorld()->SpawnActor<ANF_WeaponBase>(
		SelectedWeapon->WeaponClass, GetOwner()->GetActorTransform(), Parameters);
	if (!NewWeapon)
	{
		return false;
	}
	if (!FNF_BuildEquipmentAdapter::ApplyWeaponData(NewWeapon, *SelectedWeapon))
	{
		UE_LOG(LogTemp, Warning, TEXT("빌드 무기 DA 적용 실패: ItemId=%s, Source=%s, Class=%s. "
			       "공용 BP의 티어 교체에는 INF_BuildWeaponDataReceiver 구현이 필요합니다."),
		       *SelectedWeaponId.ToString(), *GetNameSafe(SelectedWeapon->WeaponData.Get()),
		       *GetNameSafe(SelectedWeapon->WeaponClass.Get()));
		NewWeapon->Destroy();
		return false;
	}
	ANF_WeaponBase* PreviousWeapon = Weapons->GetEquippedWeapon();
	if (!Weapons->EquipWeapon(NewWeapon))
	{
		NewWeapon->Destroy();
		return false;
	}
	ANF_WeaponBase* PreviousBuildWeapon = BuildWeapon.Get();
	BuildWeapon = NewWeapon;
	BuildWeaponItemId = SelectedWeaponId;
	// 상점 교체가 성공하면 분리된 이전 무기를 제거한다. 기본 무기도 제거 대상에 포함한다.
	if (IsValid(PreviousWeapon))
	{
		PreviousWeapon->Destroy();
	}
	if (IsValid(PreviousBuildWeapon) && PreviousBuildWeapon != PreviousWeapon)
	{
		PreviousBuildWeapon->Destroy();
	}
	return true;
}

void UNF_BuildComponent::ReleaseBuildWeapon()
{
	if (ANF_WeaponBase* Weapon = BuildWeapon.Get())
	{
		if (UNF_WeaponComponent* Weapons = GetOwner()->FindComponentByClass<UNF_WeaponComponent>())
		{
			if (Weapons->GetEquippedWeapon() == Weapon)
			{
				Weapons->UnequipWeapon();
			}
		}
		Weapon->Destroy();
	}
	BuildWeapon.Reset();
	BuildWeaponItemId = NAME_None;
}

bool UNF_BuildComponent::GrantItem(
	UNF_BuildItemDefinitionDataAsset* Definition, UObject* ItemSource, const FNF_BuildItemData& Data, int32 SlotIndex,
	FNF_BuildGrantHandles& OutHandles)
{
	if (!IsValid(Definition))
	{
		return false;
	}
	
	OutHandles.Definition = Definition;
	OutHandles.ItemData = Data;
	
	const bool bIsSkill=
			Data.ItemType == ENF_ItemType::ActiveSkill || Data.ItemType == ENF_ItemType::PassiveSkill;
	
	const int32 InputId = 
		Data.ItemType == ENF_ItemType::ActiveSkill ? SlotIndex : INDEX_NONE;
	
	if (bIsSkill)
	{
		UNF_SkillDataAsset* SkillData = Cast<UNF_SkillDataAsset>(ItemSource);
		
		if (!IsValid(SkillData))
		{
			return false;
		}
		
		OutHandles.AbilityHandle = SkillData->GrantTo(AbilitySystem, InputId);
		
		if (!OutHandles.AbilityHandle.IsValid())
		{
			return false;
		}
	}
	else if (Data.AbilityClass)
	{
		OutHandles.AbilityHandle = AbilitySystem->GiveAbility(
			FGameplayAbilitySpec(Data.AbilityClass,Data.AbilityLevel, InputId, Definition));
		
		if (!OutHandles.AbilityHandle.IsValid())
		{
			return false;
		}
	}
	

	for (const TSubclassOf<UGameplayEffect>& EffectClass : Data.EquippedEffects)
	{
		for (const FGameplayModifierInfo& Modifier : EffectClass->GetDefaultObject<UGameplayEffect>()->Modifiers)
		{
			if (!Modifier.Attribute.IsValid() || !AbilitySystem->HasAttributeSetForAttribute(Modifier.Attribute))
			{
				return false;
			}
		}
		FGameplayEffectContextHandle Context = AbilitySystem->MakeEffectContext();
		Context.AddSourceObject(Definition);
		const FGameplayEffectSpecHandle Spec =
			AbilitySystem->MakeOutgoingSpec(EffectClass, Data.AbilityLevel, Context);
		if (!Spec.IsValid())
		{
			return false;
		}
		const FActiveGameplayEffectHandle EffectHandle = AbilitySystem->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		if (!EffectHandle.IsValid())
		{
			return false;
		}
		OutHandles.EffectHandles.Add(EffectHandle);
	}
	return true;
}

void UNF_BuildComponent::RevokeItem(const FNF_BuildGrantHandles& Handles)
{
	if (!IsValid(AbilitySystem))
	{
		return;
	}
	if (Handles.AbilityHandle.IsValid())
	{
		AbilitySystem->CancelAbilityHandle(Handles.AbilityHandle);
		AbilitySystem->ClearAbility(Handles.AbilityHandle);
	}
	for (const FActiveGameplayEffectHandle& EffectHandle : Handles.EffectHandles)
	{
		AbilitySystem->RemoveActiveGameplayEffect(EffectHandle);
	}
}

void UNF_BuildComponent::OnRep_CurrentBuild()
{
	// 적용 결과를 기다릴 때만 확정된 빌드로 맞춘다. 상점을 연 동안 이전 빌드가 다시 채워지면 안 된다.
	if (CurrentShop && bAwaitingBuildResult)
	{
		DraftBuild = CurrentBuild;
		OnDraftChanged.Broadcast();
	}
	OnBuildChanged.Broadcast();
}

void UNF_BuildComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CurrentShop = nullptr;
	if (GetOwner()->HasAuthority())
	{
		if (bOwnsReadyTag && ReadyAbilitySystem.IsValid())
		{
			ReadyAbilitySystem->RemoveLooseGameplayTag(
				NFTag_Build::State_Build_Ready, 1, EGameplayTagReplicationState::CountToOwner);
			bOwnsReadyTag = false;
			ReadyAbilitySystem.Reset();
		}
		ReleaseBuildWeapon();
		ReleaseBuildArmors();
		for (const TPair<FName, FNF_BuildGrantHandles>& Item : AppliedItems)
		{
			RevokeItem(Item.Value);
		}
		AppliedItems.Empty();
	}
	Super::EndPlay(EndPlayReason);
}

void UNF_BuildComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UNF_BuildComponent, CurrentBuild, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UNF_BuildComponent, PlayerLevel, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UNF_BuildComponent, EquippedItems, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UNF_BuildComponent, CurrentShop, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UNF_BuildComponent, ShopsInRange, COND_OwnerOnly);


	DOREPLIFETIME_CONDITION(UNF_BuildComponent, CostLimit, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UNF_BuildComponent, bEntitlementsInitialized, COND_OwnerOnly);
}

bool UNF_BuildComponent::UpdateBuildCostLimit(int32 NewCostLimit)
{
	if (!IsValid(GetOwner()) ||
		!GetOwner()->HasAuthority() ||
		bApplyingBuild ||
		NewCostLimit < 0)
	{
		return false;
	}

	CostLimit = NewCostLimit;
	bEntitlementsInitialized = true;

	GetOwner()->ForceNetUpdate();
	OnRep_BuildEntitlements();
	return true;
}

void UNF_BuildComponent::OnRep_BuildEntitlements()
{
	OnBuildEntitlementsChanged.Broadcast();
	OnDraftChanged.Broadcast();
}

UNF_BuildRules* UNF_BuildComponent::GetRules() const
{
	return IsValid(CurrentShop) ? CurrentShop->GetRules() : nullptr;
}

UDataAsset* UNF_BuildComponent::GetItemSource(FName ItemId) const
{
	if (const UNF_BuildRules* Rules = GetRules())
	{
		if (Rules->FindItemDefinition(ItemId)) { return Rules->ItemSources.FindRef(ItemId); }
	}
	for (const FNF_BuildEquippedItem& Item : EquippedItems)
	{
		if (IsValid(Item.Definition) && Item.Definition->ItemId == ItemId) { return Item.Source; }
	}
	return nullptr;
}
