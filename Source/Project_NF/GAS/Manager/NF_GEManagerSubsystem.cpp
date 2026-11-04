#include "NF_GEManagerSubsystem.h"
#include "NF_GEManagerSettings.h"
#include "GAS/Data/NF_GERootDataAsset.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

void UNF_GEManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	const UNF_GEManagerSettings* Settings = GetDefault<UNF_GEManagerSettings>();
	
	if (!IsValid(Settings) || Settings->RootGEDataAsset.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("[GEManager] Root GE DataAsset이 설정되지 않았습니다"));
		return;
	}
	
	LoadedRootGEDataAsset = Settings->RootGEDataAsset.LoadSynchronous();
	
	if (!IsValid(LoadedRootGEDataAsset))
	{
		UE_LOG(LogTemp, Error, TEXT("[GEManager] Root GE DataAsset 로드에 실패했습니다"));
		return;
	}
	
	UE_LOG(LogTemp, Log, TEXT("[GEManager] Root GE DataAsset 로드: %s"), *GetNameSafe(LoadedRootGEDataAsset));
}

void UNF_GEManagerSubsystem::Deinitialize()
{
	LoadedRootGEDataAsset = nullptr;
	Super::Deinitialize();
}

UNF_GEManagerSubsystem* UNF_GEManagerSubsystem::Get(const UObject* WorldContextObject)
{
	if (!IsValid(WorldContextObject))
	{
		return nullptr;
	}
	
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject);
	
	if (!IsValid(GameInstance))
	{
		return nullptr;
	}
	
	return  GameInstance->GetSubsystem<UNF_GEManagerSubsystem>();
}

const UNF_DamageGEDataAsset* UNF_GEManagerSubsystem::GetDamageGEDataAsset() const
{
	return IsValid(LoadedRootGEDataAsset) ? LoadedRootGEDataAsset->DamageGEDataAsset.Get() : nullptr;
}	

const UNF_RewardGEDataAsset* UNF_GEManagerSubsystem::GetRewardGEDataAsset() const
{
	return IsValid(LoadedRootGEDataAsset) ? LoadedRootGEDataAsset->RewardGEDataAsset.Get() : nullptr;
}

const UNF_SurvivalGEDataAsset* UNF_GEManagerSubsystem::GetSurvivalGEDataAsset() const
{
	return IsValid(LoadedRootGEDataAsset) ? LoadedRootGEDataAsset->SurvivalGEDataAsset.Get() : nullptr;
}

const UNF_PlayerGEDataAsset* UNF_GEManagerSubsystem::GetPlayerGEDataAsset() const
{
	return IsValid(LoadedRootGEDataAsset) ? LoadedRootGEDataAsset->PlayerGEDataAsset.Get() : nullptr;
}

const UNF_MonsterGEDataAsset* UNF_GEManagerSubsystem::GetMonsterGEDataAsset() const
{
	return IsValid(LoadedRootGEDataAsset) ? LoadedRootGEDataAsset->MonsterGEDataAsset.Get() : nullptr;
}

const UNF_BossGEDataAsset* UNF_GEManagerSubsystem::GetBossGEDataAsset() const
{
	return IsValid(LoadedRootGEDataAsset) ? LoadedRootGEDataAsset->BossGEDataAsset.Get() : nullptr;
}

const UNF_SkillGEDataAsset* UNF_GEManagerSubsystem::GetSkillGEDataAsset() const
{
	return IsValid(LoadedRootGEDataAsset) ? LoadedRootGEDataAsset->SkillGEDataAsset.Get() : nullptr;
}

const UNF_EquipmentGEDataAsset* UNF_GEManagerSubsystem::GetEquipmentGEDataAsset() const
{
	return IsValid(LoadedRootGEDataAsset) ? LoadedRootGEDataAsset->EquipmentGEDataAsset.Get() : nullptr;
}

const UNF_ConsumableGEDataAsset* UNF_GEManagerSubsystem::GetConsumableGEDataAsset() const
{
	return IsValid(LoadedRootGEDataAsset) ? LoadedRootGEDataAsset->ConsumableGEDataAsset.Get() : nullptr;
}