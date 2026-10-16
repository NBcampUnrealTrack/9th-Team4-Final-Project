#include "BuildSystem/Player/NF_BuildComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "BuildSystem/Core/NF_BuildValidator.h"
#include "BuildSystem/Data/NF_BuildItemDefinitionDATAASSET.h"
#include "BuildSystem/Data/NF_BuildRules.h"
#include "GAS/Tags/NFTag_Build.h"
#include "BuildSystem/GamePlay/BuildShop/NF_BuildShopZone.h"
#include "BuildSystem/UI/NF_BuildShopWidget.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffect.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"
#include "GamePlay/Combat/Weapons/NF_WeaponComponent.h"
#include "Net/UnrealNetwork.h"

UNF_BuildComponent::UNF_BuildComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickInterval = 0.2f;
	SetIsReplicatedByDefault(true);
	BuildWidgetClass = UNF_BuildShopWidget::StaticClass();
}

void UNF_BuildComponent::BeginPlay()
{
	Super::BeginPlay();
	SetComponentTickEnabled(GetOwner()->HasAuthority());
	if (GetOwner()->HasAuthority())
	{
		UpdateShopsInRange();
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

UNF_BuildItemDefinitionDATAASSET* UNF_BuildComponent::FindItemDefinition(FName ItemId) const
{
	if (Rules)
	{
		for (UNF_BuildItemDefinitionDATAASSET* Definition : Rules->Catalog)
		{
			if (IsValid(Definition) && Definition->ItemId == ItemId)
			{
				return Definition;
			}
		}
	}
	return nullptr;
}

TArray<UNF_BuildItemDefinitionDATAASSET*> UNF_BuildComponent::GetCatalogItems(
	ENF_BuildItemType ItemType, ENF_BuildArmorPart ArmorPart) const
{
	TArray<UNF_BuildItemDefinitionDATAASSET*> Result;
	if (Rules)
	{
		for (UNF_BuildItemDefinitionDATAASSET* Definition : Rules->Catalog)
		{
			if (IsValid(Definition) && Definition->ItemType == ItemType &&
				(ItemType != ENF_BuildItemType::Armor || ArmorPart == ENF_BuildArmorPart::None ||
					Definition->ArmorPart == ArmorPart))
			{
				Result.Add(Definition);
			}
		}
	}
	return Result;
}

UNF_BuildItemDefinitionDATAASSET* UNF_BuildComponent::GetSlotItem(
	ENF_BuildItemType ItemType, int32 SlotIndex, bool bUseDraft) const
{
	const TArray<FNF_BuildSelection>& Selections = bUseDraft ? DraftBuild : CurrentBuild;
	for (const FNF_BuildSelection& Selection : Selections)
	{
		UNF_BuildItemDefinitionDATAASSET* Definition = FindItemDefinition(Selection.ItemId);
		if (Definition && Definition->ItemType == ItemType && Selection.SlotIndex == SlotIndex &&
			Definition->CanUseSlot(SlotIndex))
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
	const UNF_BuildItemDefinitionDATAASSET* Definition = FindItemDefinition(ItemId);
	if (!CanEditDraft() || !Definition || Rules->GetSlotCount(Definition->ItemType) < 1)
	{
		return false;
	}
	PendingActiveSkill = Definition->ItemType == ENF_BuildItemType::ActiveSkill ? ItemId : NAME_None;
	if (PendingActiveSkill.IsNone())
	{
		const int32 TargetSlotIndex = Definition->GetDefaultSlotIndex();
		if (!Definition->CanUseSlot(TargetSlotIndex) || TargetSlotIndex >= Rules->GetSlotCount(Definition->ItemType))
		{
			return false;
		}
		DraftBuild.RemoveAll([this, Definition, TargetSlotIndex](const FNF_BuildSelection& Selection)
		{
			const UNF_BuildItemDefinitionDATAASSET* Existing = FindItemDefinition(Selection.ItemId);
			return Existing && Existing->ItemType == Definition->ItemType && Selection.SlotIndex == TargetSlotIndex;
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
	const UNF_BuildItemDefinitionDATAASSET* Definition = FindItemDefinition(PendingActiveSkill);
	if (!CanEditDraft() || !Definition || Definition->ItemType != ENF_BuildItemType::ActiveSkill ||
		SlotIndex < 0 || SlotIndex > 1 || SlotIndex >= Rules->ActiveSkillSlots)
	{
		return false;
	}
	DraftBuild.RemoveAll([this, SlotIndex](const FNF_BuildSelection& Selection)
	{
		const UNF_BuildItemDefinitionDATAASSET* Existing = FindItemDefinition(Selection.ItemId);
		return Selection.ItemId == PendingActiveSkill ||
			(Existing && Existing->ItemType == ENF_BuildItemType::ActiveSkill && Selection.SlotIndex == SlotIndex);
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

void UNF_BuildComponent::PressActiveSkillInput(int32 SlotIndex)
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	UAbilitySystemComponent* ASC = ResolveAbilitySystem();
	if (!Pawn || !Pawn->IsLocallyControlled() || SlotIndex < 0 || SlotIndex > 1)
	{
		return;
	}
	if (IsValid(CurrentShop))
	{
		AssignSelectedActiveSkill(SlotIndex);
		return;
	}
	if (ASC && Rules && SlotIndex < Rules->ActiveSkillSlots)
	{
		ASC->AbilityLocalInputPressed(SlotIndex + 1);
	}
}

void UNF_BuildComponent::ReleaseActiveSkillInput(int32 SlotIndex)
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	UAbilitySystemComponent* ASC = ResolveAbilitySystem();
	if (Pawn && Pawn->IsLocallyControlled() && ASC && SlotIndex >= 0 && SlotIndex <= 1)
	{
		ASC->AbilityLocalInputReleased(SlotIndex + 1);
	}
}

void UNF_BuildComponent::OnRep_CurrentShop()
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (Pawn && Pawn->IsLocallyControlled())
	{
		ReleaseActiveSkillInput(0);
		ReleaseActiveSkillInput(1);
		DraftBuild = CurrentBuild;
		PendingActiveSkill = NAME_None;
		bDraftDirty = false;
		UpdateLocalShopUI();
		OnDraftChanged.Broadcast();
	}

	const ENF_BuildShopUIState SessionState =
	IsValid(CurrentShop)
		? ENF_BuildShopUIState::BuildOpened
		: ENF_BuildShopUIState::BuildClosed;

	OnBuildShopChanged.Broadcast(SessionState);
}

void UNF_BuildComponent::OnRep_BuildConfiguration()
{
	OnDraftChanged.Broadcast();
}

void UNF_BuildComponent::UpdateLocalShopUI()
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* Controller = BuildWidget
		                                ? BuildWidget->GetOwningPlayer()
		                                : Pawn
		                                ? Cast<APlayerController>(Pawn->GetController())
		                                : nullptr;
	if (!Controller || !Controller->IsLocalController() || !Controller->GetLocalPlayer())
	{
		return;
	}
	if (IsValid(CurrentShop) && bAutomaticallyShowWidget && BuildWidgetClass && !BuildWidget)
	{
		BuildWidget = CreateWidget<UUserWidget>(Controller, BuildWidgetClass);
		if (BuildWidget)
		{
			bSavedMouseCursor = Controller->bShowMouseCursor;
			BuildWidget->AddToViewport();
			FInputModeGameAndUI Mode;
			Mode.SetWidgetToFocus(BuildWidget->TakeWidget());
			Mode.SetHideCursorDuringCapture(false);
			Controller->SetInputMode(Mode);
			Controller->bShowMouseCursor = true;
		}
	}
	else if (!IsValid(CurrentShop) && BuildWidget)
	{
		BuildWidget->RemoveFromParent();
		BuildWidget = nullptr;
		Controller->SetInputMode(FInputModeGameOnly());
		Controller->bShowMouseCursor = bSavedMouseCursor;
	}
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
	GetOwner()->ForceNetUpdate();
	OnDraftChanged.Broadcast();
	return true;
}

bool UNF_BuildComponent::SetPlayerLevel(int32 NewLevel)
{
	int32 CostLimit = 0;
	if (!GetOwner()->HasAuthority() || bApplyingBuild || !Rules || !Rules->TryGetCostLimit(NewLevel, CostLimit))
	{
		return false;
	}
	if (!CurrentBuild.IsEmpty() &&
		FNF_BuildValidator::Validate(Rules, NewLevel, CurrentBuild).Result != ENF_BuildResult::Success)
	{
		return false;
	}
	PlayerLevel = NewLevel;
	GetOwner()->ForceNetUpdate();
	OnDraftChanged.Broadcast();
	return true;
}

FNF_BuildValidation UNF_BuildComponent::PreviewBuild(const TArray<FNF_BuildSelection>& Selections) const
{
	return FNF_BuildValidator::Validate(Rules, PlayerLevel, Selections);
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
	const TArray<FNF_BuildSelection>& Selections, ANF_BuildShopZone* ShopZone)
{
	FNF_BuildValidation Validation;
	auto Fail = [&Validation](ENF_BuildResult Result, const TCHAR* Message)
	{
		Validation.Result = Result;
		Validation.Message = FText::FromString(Message);
		return Validation;
	};

	if (!GetOwner()->HasAuthority())
	{
		return Fail(ENF_BuildResult::NotAuthority, TEXT("빌드 적용은 서버에서만 가능합니다."));
	}
	if (bApplyingBuild)
	{
		return Fail(ENF_BuildResult::BuildBlocked, TEXT("빌드 적용이 진행 중입니다."));
	}
	TGuardValue<bool> ApplyGuard(bApplyingBuild, true);
	if (!IsValid(AbilitySystem) || AbilitySystem->GetAvatarActor() != GetOwner())
	{
		return Fail(
			ENF_BuildResult::MissingAbilitySystem, TEXT("플레이어 ASC의 ActorInfo와 빌드 컴포넌트를 연결해야 합니다."));
	}
	if (AbilitySystem->HasMatchingGameplayTag(NFTag_Build::State_Build_Locked))
	{
		return Fail(ENF_BuildResult::BuildBlocked, TEXT("현재 상태에서는 빌드를 교체할 수 없습니다."));
	}
	const APawn* PlayerPawn = Cast<APawn>(GetOwner());
	if (!IsValid(ShopZone) || CurrentShop != ShopZone || !SessionController.IsValid() || !PlayerPawn ||
		PlayerPawn->GetController() != SessionController.Get() || ShopZone->GetWorld() != GetWorld() || !ShopZone->
		CanBuild(GetOwner()))
	{
		return Fail(ENF_BuildResult::NotInBuildZone, TEXT("상점과 먼저 상호작용하고 사용 거리 안에 있어야 합니다."));
	}

	Validation = PreviewBuild(Selections);
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
		UNF_BuildItemDefinitionDATAASSET* Definition = nullptr;
		for (UNF_BuildItemDefinitionDATAASSET* Candidate : Rules->Catalog)
		{
			if (Candidate->ItemId == Selection.ItemId)
			{
				Definition = Candidate;
				break;
			}
		}
		FNF_BuildGrantHandles Handles;
		if (!GrantItem(Definition, Selection.SlotIndex, Handles))
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
	if (!ApplyWeaponSelection(Selections))
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
		if (Handles.Definition->ItemType == ENF_BuildItemType::ActiveSkill)
		{
			if (FGameplayAbilitySpec* Spec = AbilitySystem->FindAbilitySpecFromHandle(Handles.AbilityHandle))
			{
				// Input IDs 1..N are reserved for active build slots. Slot movement keeps the spec.
				Spec->InputID = Selection.SlotIndex + 1;
				AbilitySystem->MarkAbilitySpecDirty(*Spec);
			}
		}
	}

	CurrentBuild = Selections;
	GetOwner()->ForceNetUpdate();
	OnRep_CurrentBuild();
	Validation.Message = FText::FromString(TEXT("빌드를 적용했습니다."));
	return Validation;
}

// @Owner: 황준현 | 2026-10-15. 빌드는 무기 선택과 생성을, 무기 컴포넌트는 장착과 전투를 담당한다.
bool UNF_BuildComponent::ApplyWeaponSelection(const TArray<FNF_BuildSelection>& Selections)
{
	UNF_BuildItemDefinitionDATAASSET* SelectedWeapon = nullptr;
	for (const FNF_BuildSelection& Selection : Selections)
	{
		UNF_BuildItemDefinitionDATAASSET* Definition = FindItemDefinition(Selection.ItemId);
		if (Definition && Definition->ItemType == ENF_BuildItemType::Weapon && Definition->WeaponClass)
		{
			// 팀 공용 무기 컴포넌트는 무기 한 개의 장착만 지원한다.
			if (SelectedWeapon || Selection.SlotIndex != 0)
			{
				return false;
			}
			SelectedWeapon = Definition;
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
	if (BuildWeapon.IsValid() && BuildWeaponItemId == SelectedWeapon->ItemId &&
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
	BuildWeaponItemId = SelectedWeapon->ItemId;
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
	UNF_BuildItemDefinitionDATAASSET* Definition, int32 SlotIndex, FNF_BuildGrantHandles& OutHandles)
{
	if (!Definition)
	{
		return false;
	}
	OutHandles.Definition = Definition;
	if (Definition->AbilityClass)
	{
		const int32 InputId = Definition->ItemType == ENF_BuildItemType::ActiveSkill ? SlotIndex + 1 : INDEX_NONE;
		OutHandles.AbilityHandle = AbilitySystem->GiveAbility(
			FGameplayAbilitySpec(Definition->AbilityClass, Definition->AbilityLevel, InputId, Definition));
		if (!OutHandles.AbilityHandle.IsValid())
		{
			return false;
		}
	}

	for (const TSubclassOf<UGameplayEffect>& EffectClass : Definition->EquippedEffects)
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
			AbilitySystem->MakeOutgoingSpec(EffectClass, Definition->AbilityLevel, Context);
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
	UpdateLocalShopUI();
	if (GetOwner()->HasAuthority())
	{
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
}
