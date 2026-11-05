#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "NF_Monster.generated.h"

class UNF_AbilitySystemComponent;
class UNF_MonsterAttributeSet;
class UGameplayEffect;
class UGameplayAbility;
class UAnimMontage;
struct FOnAttributeChangeData;
class ANF_Monster;
class AActor;
class ANF_MonsterItemDrop;
class ANF_MonsterSpawner;
class UWidgetComponent;
class APlayerState;
class UNF_MonsterStatusWidget;

//사망 알림 deligate
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FNFOnMonsterDied,
	ANF_Monster*,
	DeadMonster);

USTRUCT(BlueprintType)
struct FNF_MonsterDropEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "NF|Monster|Drop")
	TSubclassOf<AActor> DropActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "NF|Monster|Drop",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DropChance = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "NF|Monster|Drop",
		meta = (ClampMin = "1"))
	int32 MinQuantity = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "NF|Monster|Drop",
		meta = (ClampMin = "1"))
	int32 MaxQuantity = 1;
};

UCLASS()
class PROJECT_NF_API ANF_Monster : public ACharacter , public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ANF_Monster();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** Maximum target distance at which a basic-attack hit can be applied. */
	float GetBasicAttackHitRange() const { return BasicAttackHitRange; }
	
	void ApplyBasicAttackHit();
	
	void ApplyLandingDamage(
	const FVector& LandingLocation,
	float DamageRadius,
	float DamageMultiplier);
	
	UPROPERTY(BlueprintAssignable, Category = "NF|Monster|Death")
	FNFOnMonsterDied OnMonsterDied;
	
	FVector GetHomeLocation() const;

	void ApplyGroupDamageAggro(AActor* TargetActor);

	void SetGuardFocusActor(AActor* InGuardFocusActor);

	void SetGuardSpawner(ANF_MonsterSpawner* InGuardSpawner);

	AActor* GetGuardFocusActor() const { return GuardFocusActor.Get(); }

	bool RequiresDamageToAggro() const { return bRequiresDamageToAggro; }

	UFUNCTION(BlueprintPure, Category = "NF|Monster|Death")
	bool IsDead() const { return bIsDead; }
	
	UFUNCTION(BlueprintImplementableEvent, Category = "NF|Monster|Death")
	void StartCorpseRemovalVfx(float CorpseDuration);

	UFUNCTION(BlueprintPure, Category = "NF|Monster|UI")
	FText GetMonsterDisplayName() const { return MonsterDisplayName; }
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void UpdateLocalStatusVisibility();

	UFUNCTION()
	void OnRep_MonsterDisplayName();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastRevealStatusToAttacker(APlayerState* Attacker);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_MonsterDisplayName, Category = "NF|Monster|UI")
	FText MonsterDisplayName;

	/** Assign a Widget Blueprint derived from NF_MonsterStatusWidget. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Monster|UI")
	TSubclassOf<UNF_MonsterStatusWidget> MonsterStatusWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Monster|UI")
	TObjectPtr<UWidgetComponent> MonsterStatusWidgetComponent;

	UPROPERTY(EditDefaultsOnly, Category = "NF|Monster|UI", meta = (ClampMin = "0.0", Units = "cm"))
	float StatusVisibleDistance = 1000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "NF|Monster|UI", meta = (ClampMin = "0.0", Units = "s"))
	float StatusDamageVisibleDuration = 4.0f;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Monster|GAS")
	TObjectPtr<UNF_AbilitySystemComponent> MonsterASC;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Monster|GAS")
	TObjectPtr<UNF_MonsterAttributeSet> MonsterAttributeSet;	
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Monster|GAS")
	TSubclassOf<UGameplayEffect> DefaultAttributesEffect;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Monster|GAS")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;
	
	UPROPERTY(EditDefaultsOnly, Category = "NF|Monster|Combat")
	TSubclassOf<UGameplayEffect> BasicAttackDamageEffect;

	UPROPERTY(EditDefaultsOnly, Category = "NF|Monster|Combat",
		meta = (ClampMin = "0.0"))
	float BasicAttackHitRange = 250.0f;
	
	//hit
	UFUNCTION()
	void HandleMonsterDamaged(float Damage,	AActor* DamageSource);

	UFUNCTION()
	void ClearHitReactState();

	UPROPERTY(EditDefaultsOnly, Category = "NF|Monster|Combat")
	TObjectPtr<UAnimMontage> HitReactMontage;

	FTimerHandle HitReactTimerHandle;
	
	//death
	UFUNCTION()
	void HandleMonsterOutOfHealth();

	UFUNCTION()
	void OnRep_IsDead();

	UPROPERTY(EditDefaultsOnly, Category = "NF|Monster|Combat")
	TObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY(EditDefaultsOnly, Category = "NF|Monster|Combat",
		meta = (ClampMin = "0.0"))
	float CorpseLifeSpan = 5.0f;

	// 몬스터 종류별로 BP에서 설정할 경험치 보상
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "NF|Monster|Reward", meta = (ClampMin = "0.0"))
	float ExperienceReward = 30.0f;

	// IncomingExp를 증가시키는 GE
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "NF|Monster|Reward")
	TSubclassOf<UGameplayEffect> ExperienceRewardEffect;

	// 경험치 GE의 SetByCaller Data Tag와 동일하게 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "NF|Monster|Reward")
	FGameplayTag ExperienceRewardMagnitudeTag;
	
	UPROPERTY(ReplicatedUsing = OnRep_IsDead)
	bool bIsDead = false;
	
	//몬스터 종류별 MoveSpeed
	void HandleMoveSpeedChanged(
	const FOnAttributeChangeData& Data);

	void SetMonsterMoveSpeed(float NewMoveSpeed);
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "NF|Monster|AI",
		meta = (ClampMin = "0.0"))
	float PatrolRadius = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "NF|Monster|Guard")
	bool bRequiresDamageToAggro = false;

	UPROPERTY(Transient)
	TObjectPtr<AActor> GuardFocusActor;

	TWeakObjectPtr<ANF_MonsterSpawner> GuardSpawner;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
	Category = "NF|Monster|Drop")
	TArray<FNF_MonsterDropEntry> DropEntries;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "NF|Monster|Drop",
		meta = (ClampMin = "0.0", Units = "cm"))
	float DropScatterRadius = 80.0f;
	
private:
	FVector HomeLocation;
	FTimerHandle StatusVisibilityTimerHandle;
	float StatusRevealedUntil = 0.0f;
	
	void GrantExperienceToKiller();
	void SpawnMonsterDrops();
	bool FindMonsterDropLocation(FVector& OutDropLocation) const;
};
