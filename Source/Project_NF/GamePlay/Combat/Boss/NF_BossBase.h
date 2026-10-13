#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"
#include "GameplayCueInterface.h"
#include "NF_BossBase.generated.h"

class UAbilitySystemComponent;
class UStaticMeshComponent;
class USceneComponent;
class UGameplayAbility;
class ANF_BossTelegraphMarker;
class UMaterialInterface;

/** Stationary GAS-enabled boss base. Boss patterns are granted as GameplayAbilities. */
UCLASS(Blueprintable)
class PROJECT_NF_API ANF_BossBase : public AActor, public IAbilitySystemInterface, public IGameplayCueInterface
{
	GENERATED_BODY()

public:
	ANF_BossBase();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void HandleGameplayCue(UObject* Self, FGameplayTag GameplayCueTag,
		EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Boss|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Boss|Components")
	TObjectPtr<UStaticMeshComponent> BossMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Boss|GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Abilities")
	TSubclassOf<UGameplayAbility> InitialPatternAbility;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Abilities")
	bool bActivateInitialPatternOnBeginPlay = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Telegraph")
	TSubclassOf<ANF_BossTelegraphMarker> TelegraphMarkerClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Telegraph")
	TObjectPtr<UMaterialInterface> TelegraphDecalMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Telegraph")
	FName TelegraphProgressParameterName = TEXT("Progress");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Telegraph", meta = (ClampMin = "0.0", Units = "cm"))
	float TelegraphProjectionDepth = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Telegraph", meta = (Units = "cm"))
	float TelegraphHeightOffset = 2.0f;
};
