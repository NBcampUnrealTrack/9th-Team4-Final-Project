#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include <type_traits>
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
class UGameplayEffect;

UCLASS()
class PROJECT_NF_API UNF_GEManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	static UNF_GEManagerSubsystem* Get(const UObject* WorldContextObject);
	
	// 카테고리별 GE DataAsset 간편 조회
	template<typename TCategory>
	static const TCategory* FindData(const UObject* WorldContextObject)
	{
		UNF_GEManagerSubsystem* Manager = Get(WorldContextObject);
		
		if (!IsValid(Manager))
		{
			return nullptr;
		}
		
		if constexpr (std::is_same_v<TCategory, UNF_DamageGEDataAsset>)
		{
			return Manager->GetDamageGEDataAsset();
		}
		else if constexpr (std::is_same_v<TCategory, UNF_RewardGEDataAsset>)
		{
			return Manager->GetRewardGEDataAsset();
		}
		else if constexpr (std::is_same_v<TCategory, UNF_SurvivalGEDataAsset>)
		{
			return Manager->GetSurvivalGEDataAsset();
		}
		else if constexpr (std::is_same_v<TCategory, UNF_PlayerGEDataAsset>)
		{
			return Manager->GetPlayerGEDataAsset();
		}
		else if constexpr (std::is_same_v<TCategory, UNF_MonsterGEDataAsset>)
		{
			return Manager->GetMonsterGEDataAsset();
		}
		else if constexpr (std::is_same_v<TCategory, UNF_BossGEDataAsset>)
		{
			return Manager->GetBossGEDataAsset();
		}
		else if constexpr (std::is_same_v<TCategory, UNF_SkillGEDataAsset>)
		{
			return Manager->GetSkillGEDataAsset();
		}
		else if constexpr (std::is_same_v<TCategory, UNF_EquipmentGEDataAsset>)
		{
			return Manager->GetEquipmentGEDataAsset();
		}
		else if constexpr (std::is_same_v<TCategory, UNF_ConsumableGEDataAsset>)
		{
			return Manager->GetConsumableGEDataAsset();
		}
		else
		{
			static_assert(!std::is_same_v<TCategory, TCategory>, "Unsupported GE DataAsset Category");
		}
	}
	
	// 카테고리별 DA에서 특정 GE 클래스를 간편 조회
	template<typename TCategory>
	static TSubclassOf<UGameplayEffect> FindEffect(
		const UObject* WorldContextObject, TSubclassOf<UGameplayEffect> TCategory::* EffectMember)
	{
		const TCategory* CategoryDA = FindData<TCategory>(WorldContextObject);
		
		if (!IsValid(CategoryDA))
		{
			return nullptr;
		}
		
		return CategoryDA->*EffectMember;
	}
	
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
