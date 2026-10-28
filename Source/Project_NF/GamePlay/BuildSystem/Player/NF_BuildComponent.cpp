#include "GamePlay/BuildSystem/Player/NF_BuildComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GamePlay/BuildSystem/Core/NF_BuildValidator.h"
#include "GamePlay/BuildSystem/Data/NF_BuildItemDefinitionDataAsset.h"
#include "GamePlay/BuildSystem/Data/NF_BuildRules.h"
#include "GAS/Tags/NFTag_Build.h"
#include "GamePlay/BuildSystem/GamePlay/BuildShop/NF_BuildShopZone.h"
#include "GamePlay/BuildSystem/UI/NF_BuildShopWidget.h"
#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffect.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"
#include "GamePlay/Combat/Weapons/NF_WeaponComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/Data/NF_PlayerActionDelegateSubsystem.h"
#include "Core/NF_GameState.h"
#include "UI/SubSystem/NF_UIManagerSubsystem.h"

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
		!bInitializeEntitlementsFromDefaults || bEntitlementsInitialized || !IsValid(Rules))
	{
		return;
	}

	if (!UpdateBuildEntitlements(InitialCostLimit, InitialUnlockedItemIds))
	{
		UE_LOG(LogTemp, Warning,
		       TEXT("빌드 초기 조건 적용 실패: %s. 초기 코스트와 Rules Catalog의 ItemId를 확인하세요."),
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
	if (Rules)
	{
		for (UNF_BuildItemDefinitionDataAsset* Definition : Rules->Catalog)
		{
			if (IsValid(Definition) && Definition->ItemId == ItemId)
			{
				return Definition;
			}
		}
	}
	return nullptr;
}

bool UNF_BuildComponent::ResolveItemData(FName ItemId, FNF_BuildItemData& OutData) const
{
	OutData = FNF_BuildItemData();
	return IsValid(Rules) && FindItemDefinition(ItemId) && Rules->ResolveItemData(ItemId, OutData);
}

TArray<UNF_BuildItemDefinitionDataAsset*> UNF_BuildComponent::GetCatalogItems(
	ENF_ItemType ItemType,
	ENF_ArmorSlot ArmorPart) const
{
	TArray<UNF_BuildItemDefinitionDataAsset*> Result;

	if (!IsValid(Rules) || !bEntitlementsInitialized)
	{
		return Result;
	}

	for (UNF_BuildItemDefinitionDataAsset* Definition : Rules->Catalog)
	{
		if (!IsValid(Definition)) { continue; }
		// 획득하지 않은 아이템은 목록에서 제외한다.
		if (!UnlockedItemIds.Contains(Definition->ItemId)) { continue; }
		FNF_BuildItemData Data;
		if (!ResolveItemData(Definition->ItemId, Data) || Data.ItemType != ItemType) { continue; }
		// 방어구는 지정한 부위만 표시한다. None이면 전체 부위.
		if (ItemType == ENF_ItemType::Armor &&
			ArmorPart != ENF_ArmorSlot::None &&
			Data.ArmorSlot != ArmorPart) { continue; }
		Result.Add(Definition);
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
	FNF_BuildItemData Data;
	if (!CanEditDraft() || !ResolveItemData(ItemId, Data) || Rules->GetSlotCount(Data.ItemType) < 1)
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
		bDraftDirty = true;
	}
	OnDraftChanged.Broadcast();
	return true;
}

bool UNF_BuildComponent::AssignSelectedActiveSkill(int32 SlotIndex)
{
	FNF_BuildItemData Data;
	if (!CanEditDraft() || !ResolveItemData(PendingActiveSkill, Data) || Data.ItemType != ENF_ItemType::ActiveSkill ||
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
		bDraftDirty = true;
		OnDraftChanged.Broadcast();
	}
	return bRemoved;
}

void UNF_BuildComponent::ApplyDraftBuild()
{
	if (CanEditDraft() && PendingActiveSkill.IsNone() && PreviewBuild(DraftBuild).Result == ENF_BuildResult::Success)
	{
		bAwaitingBuildResult = true;
		OnDraftChanged.Broadcast();
		RequestApplyBuild(DraftBuild, CurrentShop);
	}
}


void UNF_BuildComponent::OnRep_CurrentShop()
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (Pawn && Pawn->IsLocallyControlled())
	{
		DraftBuild = CurrentBuild;
		PendingActiveSkill = NAME_None;
		bDraftDirty = false;
		
		OnDraftChanged.Broadcast();
	}

	const ENF_BuildShopUIState SessionState =
		IsValid(CurrentShop)
			? ENF_BuildShopUIState::BuildOpened
			: ENF_BuildShopUIState::BuildClosed;

	OnBuildShopStateChanged.Broadcast(SessionState);

	NF_BROADCAST_TO_PLAYER_DELEGATE_OneParam(OnInteractBuildShop, SessionState);
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

bool UNF_BuildComponent::ConfigureRules(UNF_BuildRules* InRules)
{
	if (!GetOwner()->HasAuthority() || bApplyingBuild || !AppliedItems.IsEmpty() || !IsValid(InRules))
	{
		return false;
	}
	Rules = InRules;
	InitializeDefaultBuildEntitlements();
	GetOwner()->ForceNetUpdate();
	OnDraftChanged.Broadcast();
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

	return FNF_BuildValidator::Validate(
		Rules,
		CostLimit,
		UnlockedItemIds,
		Selections);
}

void UNF_BuildComponent::RequestApplyBuild(const TArray<FNF_BuildSelection>& Selections, ANF_BuildShopZone* ShopZone)
{
	if (GetOwner()->HasAuthority())
	{
		CompleteLocalBuildRequest(ApplyBuildOnServer(Selections, ShopZone));
	}
	else
	{
		ServerApplyBuild(Selections, ShopZone);
	}
}

void UNF_BuildComponent::ServerApplyBuild_Implementation(
	const TArray<FNF_BuildSelection>& Selections, ANF_BuildShopZone* ShopZone)
{
	ClientBuildRequestCompleted(ApplyBuildOnServer(Selections, ShopZone));
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
	ANF_BuildShopZone* ShopZone)
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

	return CommitBuildOnServer(Selections);
}

bool UNF_BuildComponent::ApplyFallbackBuildForMatchStart()
{
	const ANF_GameState* GameState = GetWorld() ? GetWorld()->GetGameState<ANF_GameState>() : nullptr;
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority() || bApplyingBuild ||
		!IsValid(GameState) || GameState->GetMatchPhase() != ENF_MatchPhase::Build || !IsValid(Rules))
	{
		return false;
	}
	if (!InitializeAbilitySystem(ResolveAbilitySystem()))
	{
		return false;
	}
	// 이미 유효하게 준비된 빌드는 반복 요청으로 덮어쓰지 않는다.
	if (!CurrentBuild.IsEmpty() && PreviewBuild(CurrentBuild).Result == ENF_BuildResult::Success &&
		AbilitySystem->HasMatchingGameplayTag(NFTag_Build::State_Build_Ready))
	{
		return true;
	}
	// 적용 중 콜백으로 원본 구성이 바뀌지 않도록 복사한다.
	const TArray<FNF_BuildSelection> Selections = Rules->FallbackBuild;
	const FNF_BuildValidation Validation = CommitBuildOnServer(Selections);
	const bool bSucceeded = Validation.Result == ENF_BuildResult::Success &&
		IsValid(AbilitySystem) && AbilitySystem->HasMatchingGameplayTag(NFTag_Build::State_Build_Ready);
	if (bSucceeded)
	{
		CloseBuildSession();
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
	const TArray<FNF_BuildSelection>& Selections)
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
	Validation = FNF_BuildValidator::Validate(Rules, CostLimit, UnlockedItemIds, Selections, &SelectedData);
	if (Validation.Result != ENF_BuildResult::Success)
	{
		return Validation;
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
			if (Candidate->ItemId == Selection.ItemId)
			{
				Definition = Candidate;
				break;
			}
		}
		FNF_BuildGrantHandles Handles;
		if (!GrantItem(Definition, SelectedData.FindChecked(Selection.ItemId), Selection.SlotIndex, Handles))
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

	// 기존 GAS 부여를 회수하기 전에, 실패할 수 있는 마지막 단계인 무기 장착을 수행한다.
	if (!ApplyWeaponSelection(Selections, SelectedData))
	{
		for (const TPair<FName, FNF_BuildGrantHandles>& Staged : StagedItems)
		{
			RevokeItem(Staged.Value);
		}
		return Fail(ENF_BuildResult::GrantFailed, TEXT("무기 장착에 실패했습니다. 기존 빌드를 유지합니다."));
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
		BuildWeapon->GetClass() == SelectedWeapon->WeaponClass.Get())
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
	UNF_BuildItemDefinitionDataAsset* Definition, const FNF_BuildItemData& Data, int32 SlotIndex,
	FNF_BuildGrantHandles& OutHandles)
{
	if (!Definition)
	{
		return false;
	}
	OutHandles.Definition = Definition;
	OutHandles.ItemData = Data;
	if (Data.AbilityClass)
	{
		const int32 InputId = Data.ItemType == ENF_ItemType::ActiveSkill ? SlotIndex : INDEX_NONE;
		OutHandles.AbilityHandle = AbilitySystem->GiveAbility(
			FGameplayAbilitySpec(Data.AbilityClass, Data.AbilityLevel, InputId, Definition));
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
	if (CurrentShop && (!bDraftDirty || bAwaitingBuildResult))
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
	DOREPLIFETIME_CONDITION(UNF_BuildComponent, Rules, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UNF_BuildComponent, CurrentShop, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UNF_BuildComponent, ShopsInRange, COND_OwnerOnly);


	DOREPLIFETIME_CONDITION(UNF_BuildComponent, CostLimit, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UNF_BuildComponent, UnlockedItemIds, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UNF_BuildComponent, bEntitlementsInitialized, COND_OwnerOnly);
}

bool UNF_BuildComponent::UpdateBuildEntitlements(
	int32 NewCostLimit,
	const TArray<FName>& NewUnlockedItemIds)
{
	if (!IsValid(GetOwner()) ||
		!GetOwner()->HasAuthority() ||
		bApplyingBuild ||
		NewCostLimit < 0)
	{
		return false;
	}

	TArray<FName> ValidatedIds;

	for (FName ItemId : NewUnlockedItemIds)
	{
		if (ItemId.IsNone() || !FindItemDefinition(ItemId))
		{
			return false;
		}

		ValidatedIds.AddUnique(ItemId);
	}

	CostLimit = NewCostLimit;
	UnlockedItemIds = MoveTemp(ValidatedIds);
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
