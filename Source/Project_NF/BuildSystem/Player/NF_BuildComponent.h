#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Components/ActorComponent.h"
#include "BuildSystem/Core/NF_BuildTypes.h"
#include "GameplayAbilitySpecHandle.h"
#include "NF_BuildComponent.generated.h"

class ANF_BuildShopZone;
class APlayerController;
class UAbilitySystemComponent;
class UNF_BuildItemDefinition;
class UNF_BuildRules;
class UUserWidget;

USTRUCT()
struct FNF_BuildGrantHandles
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UNF_BuildItemDefinition> Definition;

	FGameplayAbilitySpecHandle AbilityHandle;
	TArray<FActiveGameplayEffectHandle> EffectHandles;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNF_OnBuildChanged);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNF_OnBuildRequestCompleted, const FNF_BuildValidation&, Validation);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNF_OnBuildShopChanged, ANF_BuildShopZone*, Shop);

/** Attach to an owned replicated Pawn. The shop is never the client RPC owner. */
UCLASS(ClassGroup = (NF), meta = (BlueprintSpawnableComponent))
class PROJECT_NF_API UNF_BuildComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNF_BuildComponent();

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
	UNF_BuildItemDefinition* FindItemDefinition(FName ItemId) const;

	UFUNCTION(BlueprintCallable, Category = "NF|Build|Input")
	void PressActiveSkillInput(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "NF|Build|Input")
	void ReleaseActiveSkillInput(int32 SlotIndex);

	UPROPERTY(BlueprintAssignable, Category = "NF|Build")
	FNF_OnBuildShopChanged OnBuildShopChanged;

	UPROPERTY(BlueprintAssignable, Category = "NF|Build|Draft")
	FNF_OnBuildChanged OnDraftChanged;

	/** Wire an already initialized ASC. Reject replacing it while a build is equipped. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Build")
	bool InitializeAbilitySystem(UAbilitySystemComponent* InAbilitySystem);

	/** Optional runtime setup before equipping the first build. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Build")
	bool ConfigureRules(UNF_BuildRules* InRules);

	/** Called by the authoritative progression system; level/cost never come from the client. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Build")
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

	FNF_BuildValidation ApplyBuildOnServer(const TArray<FNF_BuildSelection>& Selections, ANF_BuildShopZone* ShopZone);
	bool GrantItem(UNF_BuildItemDefinition* Definition, int32 SlotIndex, FNF_BuildGrantHandles& OutHandles);
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
};
