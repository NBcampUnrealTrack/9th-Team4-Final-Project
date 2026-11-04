#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NF_GEManagerSubsystem.generated.h"

class UNF_GERootDataAsset;
class UNF_DamageGEDataAsset;
class UNF_RewardGEDataAsset;
class UNF_SurvivalGEDataAsset;
class UNF_PlayerGEDataAsset;
class UNF_MonsterGEDataAsset;
class UNF_BossGEDataAsset;
class UNF_SkillGEDataAsset;
class UNF_EquipmentGEDataAsset;
class UNF_ConsumableGEDataAsset;

UCLASS()
class PROJECT_NF_API UNF_GEManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	static UNF_GEManagerSubsystem* Get(const UObject* WorldContextObject);
	
	const UNF_DamageGEDataAsset* GetDamageGEDataAsset() const;
	const UNF_RewardGEDataAsset* GetRewardGEDataAsset() const;
	const UNF_SurvivalGEDataAsset* GetSurvivalGEDataAsset() const;
	const UNF_PlayerGEDataAsset* GetPlayerGEDataAsset() const;
	const UNF_MonsterGEDataAsset* GetMonsterGEDataAsset() const;
	const UNF_BossGEDataAsset* GetBossGEDataAsset() const;
	const UNF_SkillGEDataAsset* GetSkillGEDataAsset() const;
	const UNF_EquipmentGEDataAsset* GetEquipmentGEDataAsset() const;
	const UNF_ConsumableGEDataAsset* GetConsumableGEDataAsset() const;
	
private:
	UPROPERTY(Transient)
	TObjectPtr<UNF_GERootDataAsset> LoadedRootGEDataAsset;
};
