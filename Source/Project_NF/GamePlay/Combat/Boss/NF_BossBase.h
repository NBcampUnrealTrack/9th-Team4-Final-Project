#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"
#include "GameplayCueInterface.h"
#include "GamePlay/Combat/Boss/NF_BossCombatData.h"
#include "NF_BossBase.generated.h"

class UAbilitySystemComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class USceneComponent;
class UGameplayAbility;
class UStateTree;
class UStateTreeComponent;
class UNF_BossAttributeSet;
class ANF_Player;
class ANF_BossTelegraphMarker;
class UMaterialInterface;
class UNF_BossStateTreeAssetLibrary;
class UNF_BossRegistrySubsystem;
class UDataTable;
class UWidgetComponent;
class ANF_PlayerState;
class UNF_ActiveSkillDataAsset;

/** Stationary GAS-enabled boss base. Boss patterns are granted as GameplayAbilities. */
UCLASS(Blueprintable)
class PROJECT_NF_API ANF_BossBase : public AActor, public IAbilitySystemInterface, public IGameplayCueInterface
{
	GENERATED_BODY()
	friend class UNF_BossStateTreeAssetLibrary;
	friend class FNF_BossSkillDefinitionTest;

public:
	TSubclassOf<UGameplayAbility> GetLineStrikeAbilityClass() const;
	ANF_BossBase();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	USkeletalMeshComponent* GetBossSkeletalMesh() const;
	UFUNCTION(BlueprintPure, Category = "NF|Boss|UI")
	FText GetBossDisplayName() const { return BossDisplayName; }

	UFUNCTION(BlueprintCallable, Category = "NF|Boss|AI")
	bool ActivateNextCombatPattern();

	/** Wakes the boss on the server. The StateTree then advances from Dormant to its first combat pattern. */
	UFUNCTION(BlueprintCallable, Category = "NF|Boss|AI")
	bool WakeBoss();

	UFUNCTION(BlueprintPure, Category = "NF|Boss|AI")
	bool IsAwake() const { return bIsAwake; }

	UFUNCTION(BlueprintPure, Category = "NF|Boss|AI")
	bool IsCombatPatternActive() const;
	bool ActivateAbilityClass(TSubclassOf<UGameplayAbility> AbilityClass, bool bAllowWhenDefeated = false);
	bool IsAbilityClassActive(TSubclassOf<UGameplayAbility> AbilityClass) const;
	bool WasPatternCancelled(TSubclassOf<UGameplayAbility> AbilityClass) const;

	UFUNCTION(BlueprintPure, Category = "NF|Boss")
	bool IsDefeated() const { return bIsDefeated; }

	/** Synchronous spawn contract: GAS and the configured AI path are ready. */
	bool IsReadyForEncounter() const;

	UFUNCTION(BlueprintPure, Category = "NF|Boss")
	ANF_Player* GetHighestDamageTarget(float MaxRange) const;
	UFUNCTION(BlueprintPure, Category = "NF|Boss|Combat")
	float GetPatternDamage(ENF_BossPatternId PatternId, float FallbackDamage) const;
	bool ActivateLastGasp();
	bool IsLastGaspActive() const;

	void RecordDamage(ANF_PlayerState* PlayerState, float DamageAmount, bool bIsFatalDamage);
	void HandleHealthUpdated(float NewHealth);
	void CompleteDeathSequence();
	void SetSubmerged(bool bNewSubmerged);
	void SetFlying(bool bNewFlying);

	UFUNCTION(BlueprintPure, Category = "NF|Boss|Animation")
	bool IsFlying() const { return bIsFlying; }

	virtual void HandleGameplayCue(UObject* Self, FGameplayTag GameplayCueTag,
		EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|UI")
	FText BossDisplayName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Boss|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Hidden legacy placeholder kept so existing BP_TestBoss component overrides remain compatible. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Boss|Components|Legacy")
	TObjectPtr<UStaticMeshComponent> LegacyBossMesh;

	/** Assign the dragon skeletal mesh and its Animation Blueprint on this component in the child Blueprint. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Boss|Components")
	TObjectPtr<USkeletalMeshComponent> BossSkeletalMesh;

	/** World-space health display. Assign the shared boss health widget class in the boss Blueprint. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Boss|Components|UI")
	TObjectPtr<UWidgetComponent> BossHealthWidgetComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Boss|GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Boss|GAS")
	TObjectPtr<UNF_BossAttributeSet> BossAttributeSet;

	/** Server-side tuning table. Assign a row for this boss variant in its Blueprint defaults. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Data")
	TObjectPtr<UDataTable> BossCombatDataTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Data")
	FName BossCombatDataRowName = TEXT("DefaultBoss");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Boss|AI")
	TObjectPtr<UStateTreeComponent> StateTreeComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|AI")
	TObjectPtr<UStateTree> BossStateTree;

	/** Combat abilities chosen in order by the StateTree task. Keep the original fixed-point volley first. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|AI")
	TArray<TSubclassOf<UGameplayAbility>> CombatPatternAbilities;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|AI")
	TSubclassOf<UGameplayAbility> LastGaspAbility;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Abilities")
	TSubclassOf<UGameplayAbility> InitialPatternAbility;

	/** Optional definitions keyed by the exact GA class used by the StateTree/legacy slots.
	 * Patterns retain their own timers: bOverrideActiveSettings must remain false. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Abilities")
	TMap<TSubclassOf<UGameplayAbility>, TObjectPtr<UNF_ActiveSkillDataAsset>> PatternSkillData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Abilities", meta = (DisplayName = "Activate Initial Pattern When Woken"))
	bool bActivateInitialPatternOnBeginPlay = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Telegraph")
	TSubclassOf<ANF_BossTelegraphMarker> TelegraphMarkerClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Telegraph")
	TObjectPtr<UMaterialInterface> TelegraphDecalMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Telegraph")
	FName TelegraphProgressParameterName = TEXT("Progress");

	/** Total time the telegraph decal remains visible, independent of the attack warning duration. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Telegraph", meta = (ClampMin = "0.01", Units = "s"))
	float TelegraphDecalLifetime = 3.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Telegraph", meta = (ClampMin = "0.0", Units = "cm"))
	float TelegraphProjectionDepth = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Telegraph", meta = (Units = "cm"))
	float TelegraphHeightOffset = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Death")
	bool bDestroyWhenDeathSequenceCompletes = true;

	UPROPERTY(ReplicatedUsing = OnRep_IsDefeated, VisibleInstanceOnly, BlueprintReadOnly, Category = "NF|Boss|State")
	bool bIsDefeated = false;

	UPROPERTY(ReplicatedUsing = OnRep_IsSubmerged, VisibleInstanceOnly, BlueprintReadOnly, Category = "NF|Boss|State")
	bool bIsSubmerged = false;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "NF|Boss|State")
	bool bIsFlying = false;

	/** Server-side encounter gate. Combat starts only after WakeBoss is called. */
	UPROPERTY(Transient)
	bool bIsAwake = false;

private:
	bool GrantPatternAbilities();
	bool bPatternAbilitiesReady = false;
	UFUNCTION()
	void OnRep_IsDefeated();

	UFUNCTION()
	void OnRep_IsSubmerged();

	void ApplySubmergedPresentation();
	void BeginDeathSequence();
	void NotifyCoreBossDefeated();
	void InitializeBossHealthWidgetForLocalPlayer();

	TMap<TWeakObjectPtr<ANF_PlayerState>, float> DamageContribution;
	TWeakObjectPtr<ANF_PlayerState> LastHitPlayerState;
	FNF_BossCombatDataRow ActiveCombatData;
	bool bHasActiveCombatData = false;
	int32 NextCombatPatternIndex = 0;
	bool bDeathSequenceStarted = false;
	FTimerHandle BossHealthWidgetInitTimer;
};
