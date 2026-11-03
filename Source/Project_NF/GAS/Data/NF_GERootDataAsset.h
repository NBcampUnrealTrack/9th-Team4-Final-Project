#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NF_GERootDataAsset.generated.h"

class UNF_DamageGEDataAsset;
class UNF_RewardGEDataAsset;
class UNF_SurvivalGEDataAsset;
class UNF_PlayerGEDataAsset;
class UNF_MonsterGEDataAsset;
class UNF_BossGEDataAsset;
class UNF_SkillGEDataAsset;
class UNF_EquipmentGEDataAsset;
class UNF_ConsumableGEDataAsset;

UCLASS(BlueprintType)
class PROJECT_NF_API UNF_GERootDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameplayEffects")
	TObjectPtr<UNF_DamageGEDataAsset> DamageGEDataAsset;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameplayEffects")
	TObjectPtr<UNF_RewardGEDataAsset> RewardGEDataAsset;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameplayEffects")
	TObjectPtr<UNF_SurvivalGEDataAsset> SurvivalGEDataAsset;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameplayEffects")
	TObjectPtr<UNF_PlayerGEDataAsset> PlayerGEDataAsset;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameplayEffects")
	TObjectPtr<UNF_MonsterGEDataAsset> MonsterGEDataAsset;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameplayEffects")
	TObjectPtr<UNF_BossGEDataAsset> BossGEDataAsset;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameplayEffects")
	TObjectPtr<UNF_SkillGEDataAsset> SkillGEDataAsset;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameplayEffects")
	TObjectPtr<UNF_EquipmentGEDataAsset> EquipmentGEDataAsset;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameplayEffects")
	TObjectPtr<UNF_ConsumableGEDataAsset> ConsumableGEDataAsset;
};
