#pragma once

#include "CoreMinimal.h"
#include "BuildSystem/Core/NF_BuildTypes.h"
#include "Engine/DataAsset.h"
#include "NF_BuildItemDefinition.generated.h"

class UGameplayAbility;
class UGameplayEffect;
class ANF_WeaponBase;

/** One definition per item/tier. No mesh swapping is required by the prototype. */
UCLASS(BlueprintType)
class PROJECT_NF_API UNF_BuildItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Build")
	FName ItemId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Build")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Build")
	ENF_BuildItemType ItemType = ENF_BuildItemType::ActiveSkill;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Build", meta = (ClampMin = "1"))
	int32 Tier = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Build", meta = (ClampMin = "0"))
	int32 Cost = 0;

	/** @Owner: 황준현 | 2026-10-15. 실제 장착할 무기 클래스. 미지정 시 기존 데이터 전용 프로토타입으로 동작한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Build|Weapon")
	TSubclassOf<ANF_WeaponBase> WeaponClass;

	/** Optional until combat implementation is connected. Passive GA owns its event registration. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Build|GAS")
	TSubclassOf<UGameplayAbility> AbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Build|GAS", meta = (ClampMin = "1"))
	int32 AbilityLevel = 1;

	/** Pure, non-periodic, non-stacking Infinite modifiers. Damage/cooldowns belong to the GA. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Build|GAS")
	TArray<TSubclassOf<UGameplayEffect>> EquippedEffects;
};
