#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
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
	TSubclassOf<ANF_MonsterItemDrop> DropActorClass;

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

	AActor* GetGuardFocusActor() const { return GuardFocusActor.Get(); }

	bool RequiresDamageToAggro() const { return bRequiresDamageToAggro; }
	
protected:
	virtual void BeginPlay() override;
	
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

	UPROPERTY(EditDefaultsOnly, Category = "NF|Monster|Combat")
	TObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY(EditDefaultsOnly, Category = "NF|Monster|Combat",
		meta = (ClampMin = "0.0"))
	float CorpseLifeSpan = 5.0f;

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
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
	Category = "NF|Monster|Drop")
	TArray<FNF_MonsterDropEntry> DropEntries;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "NF|Monster|Drop",
		meta = (ClampMin = "0.0", Units = "cm"))
	float DropScatterRadius = 80.0f;
	
private:
	FVector HomeLocation;
	
	void SpawnMonsterDrops();
	bool FindMonsterDropLocation(FVector& OutDropLocation) const;
};
