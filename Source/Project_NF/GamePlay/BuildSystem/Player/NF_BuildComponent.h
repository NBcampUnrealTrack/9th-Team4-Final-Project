#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Components/ActorComponent.h"
#include "GamePlay/BuildSystem/Core/NF_BuildTypes.h"
#include "GameplayAbilitySpecHandle.h"
#include "NF_BuildComponent.generated.h"

class ANF_BuildShopZone;
class ANF_WeaponBase;
class APlayerController;
class UAbilitySystemComponent;
class UNF_BuildItemDefinitionDataAsset;
class UNF_BuildRules;
class UUserWidget;

USTRUCT()
struct FNF_BuildGrantHandles
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UNF_BuildItemDefinitionDataAsset> Definition;

	FGameplayAbilitySpecHandle AbilityHandle;
	TArray<FActiveGameplayEffectHandle> EffectHandles;

};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNF_OnBuildChanged);

// @Owner: 황준현 | 2026-10-20. 개인별 코스트 상한과 사용 가능 목록 변경 알림.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNF_OnBuildEntitlementsChanged);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNF_OnBuildRequestCompleted, const FNF_BuildValidation&, Validation);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNF_OnBuildShopRangeChanged, ANF_BuildShopZone*, Shop,
	ENF_BuildShopRangeState, RangeState);



/** Attach to an owned replicated Pawn. The shop is never the client RPC owner. */
UCLASS(ClassGroup = (NF), meta = (BlueprintSpawnableComponent))
class PROJECT_NF_API UNF_BuildComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNF_BuildComponent();

	// @Owner: 황준현 | 2026-10-20. 권한 있는 진행·획득 시스템에서 호출한다.
	// 클라이언트 RPC로 제공하지 않는다.
	bool UpdateBuildEntitlements(int32 NewCostLimit, const TArray<FName>& NewUnlockedItemIds);

	/** @Owner: 황준현 | 2026-10-19. Core 연동 객체에서만 호출하는 서버 기본 적용 경로. RPC가 아니다. */
	bool ApplyFallbackBuildForMatchStart();

	/** Called only from an authorized server interaction. */
	bool BeginBuildSession(ANF_BuildShopZone* Shop);

	UFUNCTION(BlueprintCallable, Category = "NF|Build")
	void CloseBuildSession();

	UFUNCTION(BlueprintPure, Category = "NF|Build")
	ANF_BuildShopZone* GetCurrentShop() const { return CurrentShop; }

	/** Active skills wait for a slot choice; other categories fill their first slot. */
	UFUNCTION(BlueprintCallable, Category = "NF|Build|Draft")
	bool SelectDraftItem(FName ItemId);

	UFUNCTION(BlueprintCallable, Category = "NF|Build|Draft")
	bool AssignSelectedActiveSkill(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "NF|Build|Draft")
	bool RemoveDraftItem(FName ItemId);

	UFUNCTION(BlueprintCallable, Category = "NF|Build|Draft")
	void ApplyDraftBuild();

	UFUNCTION(BlueprintPure, Category = "NF|Build|Draft")
	TArray<FNF_BuildSelection> GetDraftBuild() const { return DraftBuild; }

	UFUNCTION(BlueprintPure, Category = "NF|Build|Draft")
	FName GetPendingActiveSkill() const { return PendingActiveSkill; }

	UFUNCTION(BlueprintPure, Category = "NF|Build|Draft")
	bool IsAwaitingBuildResult() const { return bAwaitingBuildResult; }

	UFUNCTION(BlueprintPure, Category = "NF|Build")
	UNF_BuildItemDefinitionDataAsset* FindItemDefinition(FName ItemId) const;

	/** @Owner: 황준현 | 2026-10-15. UI 분류 목록. 방어구의 None은 모든 부위를 조회한다. */
	UFUNCTION(BlueprintPure, Category = "NF|Build|Slots")
	TArray<UNF_BuildItemDefinitionDataAsset*> GetCatalogItems(ENF_BuildItemType ItemType, ENF_BuildArmorPart ArmorPart) const;

	/** 빈 슬롯이면 nullptr. bUseDraft가 false이면 서버에서 확정된 빌드를 조회한다. */
	UFUNCTION(BlueprintPure, Category = "NF|Build|Slots")
	UNF_BuildItemDefinitionDataAsset* GetSlotItem(ENF_BuildItemType ItemType, int32 SlotIndex, bool bUseDraft) const;

	UPROPERTY(BlueprintAssignable, Category = "NF|Build|Shop")
	FNF_OnBuildShopRangeChanged OnBuildShopRangeChanged;

	/** 이벤트 바인딩 전부터 범위 안에 있던 상점도 조회할 수 있다. */
	UFUNCTION(BlueprintPure, Category = "NF|Build|Shop")
	TArray<ANF_BuildShopZone*> GetShopsInRange() const;

	UPROPERTY(BlueprintAssignable, Category = "NF|Build|Draft")
	FNF_OnBuildChanged OnDraftChanged;

	UPROPERTY(BlueprintAssignable, Category = "NF|Build")
	FNF_OnBuildEntitlementsChanged OnBuildEntitlementsChanged;

	/** Wire an already initialized ASC. Reject replacing it while a build is equipped. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Build")
	bool InitializeAbilitySystem(UAbilitySystemComponent* InAbilitySystem);

	/** Optional runtime setup before equipping the first build. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Build")
	bool ConfigureRules(UNF_BuildRules* InRules);

	/** 기존 호출 호환용 레벨 기록. 상한과 사용 권한은 UpdateBuildEntitlements로 갱신한다. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Build",
		meta = (DeprecatedFunction, DeprecationMessage = "Use UpdateBuildEntitlements for cost and unlock changes."))
	bool SetPlayerLevel(int32 NewLevel);

	UFUNCTION(BlueprintPure, Category = "NF|Build")
	FNF_BuildValidation PreviewBuild(const TArray<FNF_BuildSelection>& Selections) const;

	UFUNCTION(BlueprintCallable, Category = "NF|Build")
	void RequestApplyBuild(const TArray<FNF_BuildSelection>& Selections, ANF_BuildShopZone* ShopZone);

	UFUNCTION(BlueprintPure, Category = "NF|Build")
	TArray<FNF_BuildSelection> GetCurrentBuild() const
	{
		return CurrentBuild;
	}

	UFUNCTION(BlueprintPure, Category = "NF|Build")
	int32 GetPlayerLevel() const
	{
		return PlayerLevel;
	}

	UFUNCTION(BlueprintPure, Category = "NF|Build")
	UNF_BuildRules* GetRules() const
	{
		return Rules;
	}

	UPROPERTY(BlueprintAssignable, Category = "NF|Build")
	FNF_OnBuildChanged OnBuildChanged;

	UPROPERTY(BlueprintAssignable, Category = "NF|Build")
	FNF_OnBuildRequestCompleted OnBuildRequestCompleted;



protected:
	virtual void BeginPlay() override;
	virtual void
	TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_BuildConfiguration, Category = "NF|Build")
	TObjectPtr<UNF_BuildRules> Rules;

	/** Optional native prototype UI. Assign a project widget subclass or disable automatic display. */
	UPROPERTY(EditDefaultsOnly, Category = "NF|Build|UI")
	TSubclassOf<UUserWidget> BuildWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "NF|Build|UI")
	bool bAutomaticallyShowWidget = true;

private:
	// @Owner: 황준현 | 2026-10-15. 범위 상태는 사용 세션과 분리하여 소유 클라이언트에 복제한다.
	void UpdateShopsInRange();
	UFUNCTION()
	void OnRep_ShopsInRange();
	UPROPERTY(ReplicatedUsing = OnRep_ShopsInRange)
	TArray<TObjectPtr<ANF_BuildShopZone>> ShopsInRange;
	TArray<TWeakObjectPtr<ANF_BuildShopZone>> NotifiedShopsInRange;

	UFUNCTION(Server, Reliable)
	void ServerCloseBuildSession();

	UFUNCTION()
	void OnRep_CurrentShop();

	UFUNCTION()
	void OnRep_BuildConfiguration();

	UAbilitySystemComponent* ResolveAbilitySystem() const;
	bool CanEditDraft() const;
	void UpdateLocalShopUI();
	void CompleteLocalBuildRequest(const FNF_BuildValidation& Validation);

	UPROPERTY(ReplicatedUsing = OnRep_CurrentShop)
	TObjectPtr<ANF_BuildShopZone> CurrentShop;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> BuildWidget;

	UPROPERTY(Transient)
	TArray<FNF_BuildSelection> DraftBuild;

	FName PendingActiveSkill;
	bool bDraftDirty = false;
	bool bAwaitingBuildResult = false;
	bool bSavedMouseCursor = false;
	TWeakObjectPtr<APlayerController> SessionController;
	UFUNCTION(Server, Reliable)
	void ServerApplyBuild(const TArray<FNF_BuildSelection>& Selections, ANF_BuildShopZone* ShopZone);

	UFUNCTION(Client, Reliable)
	void ClientBuildRequestCompleted(const FNF_BuildValidation& Validation);

	UFUNCTION()
	void OnRep_CurrentBuild();

	// 일반 상점 요청의 접근 조건을 검사한다.
	FNF_BuildValidation ApplyBuildOnServer(const TArray<FNF_BuildSelection>& Selections, ANF_BuildShopZone* ShopZone);

	// @Owner: 황준현 | 2026-10-19
	// 접근 승인을 마친 요청의 코스트·슬롯 검증과 실제 적용을 담당한다.
	FNF_BuildValidation CommitBuildOnServer( const TArray<FNF_BuildSelection>& Selections);

	bool GrantItem(UNF_BuildItemDefinitionDataAsset* Definition, int32 SlotIndex, FNF_BuildGrantHandles& OutHandles);
	bool ApplyWeaponSelection(const TArray<FNF_BuildSelection>& Selections);
	void ReleaseBuildWeapon();
	TWeakObjectPtr<ANF_WeaponBase> BuildWeapon;
	FName BuildWeaponItemId;
	void RevokeItem(const FNF_BuildGrantHandles& Handles);

	UPROPERTY(ReplicatedUsing = OnRep_CurrentBuild)
	TArray<FNF_BuildSelection> CurrentBuild;

	UPROPERTY(ReplicatedUsing = OnRep_BuildConfiguration)
	int32 PlayerLevel = 1;

	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;

	UPROPERTY(Transient)
	TMap<FName, FNF_BuildGrantHandles> AppliedItems;

	/** GAS callbacks must not recursively change a build mid-transaction. */
	bool bApplyingBuild = false;

	void MarkBuildReady();
	// 이 컴포넌트가 추가한 태그 기여분만 종료 시 회수한다.
	TWeakObjectPtr<UAbilitySystemComponent> ReadyAbilitySystem;
	bool bOwnsReadyTag = false;

	UFUNCTION()
	void OnRep_BuildEntitlements();

	UPROPERTY(ReplicatedUsing = OnRep_BuildEntitlements)
	int32 CostLimit = 0;

	UPROPERTY(ReplicatedUsing = OnRep_BuildEntitlements)
	TArray<FName> UnlockedItemIds;

	UPROPERTY(ReplicatedUsing = OnRep_BuildEntitlements)
	bool bEntitlementsInitialized = false;
};
