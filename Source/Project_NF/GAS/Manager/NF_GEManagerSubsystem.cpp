#include "NF_GEManagerSubsystem.h"
#include "NF_GEManagerSettings.h"
#include "GAS/Data/NF_GERootDataAsset.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DataAsset.h"
#include "GameplayEffect.h"
#include "UObject/UnrealType.h"

namespace
{
	void ValidateGEDataAssets(const UNF_GERootDataAsset* RootDA)
	{
		checkf(IsValid(RootDA), TEXT("[GEManager] Root GE DataAsset이 유효하지 않습니다."));
		
		for (TFieldIterator<FProperty> RootIt(RootDA->GetClass(), 
			EFieldIteratorFlags::ExcludeSuper);
			RootIt; ++RootIt)
		{
			const FObjectPropertyBase* CategoryProperty = CastField<FObjectPropertyBase>(*RootIt);
			
			if (!CategoryProperty || !CategoryProperty->PropertyClass || 
				!CategoryProperty->PropertyClass->IsChildOf(UDataAsset::StaticClass()))
			{
				continue;
			}
			
			const UObject* CategoryDA = CategoryProperty->GetObjectPropertyValue_InContainer(RootDA);
			
			checkf(IsValid(CategoryDA), TEXT("[GEManager] RootDA = %s / 카테고리 DA 누락: %s"),
				*GetNameSafe(RootDA), *CategoryProperty->GetName());
			
			for (TFieldIterator<FProperty> EffectIt(CategoryDA->GetClass(), 
				EFieldIteratorFlags::ExcludeSuper);
				EffectIt; ++EffectIt)
			{
				const FClassProperty* EffectProperty = CastField<FClassProperty>(*EffectIt);
				
				if (!EffectProperty || !EffectProperty->MetaClass || !EffectProperty->MetaClass->IsChildOf(
				UGameplayEffect::StaticClass()))
				{
					continue;
				}
				
				const UObject* EffectClass = EffectProperty->GetObjectPropertyValue_InContainer(CategoryDA);
				
				checkf(IsValid(EffectClass), TEXT("[GEManager] DA = %s / GE 클래스 누락: %s"),
					*GetNameSafe(CategoryDA), *EffectProperty->GetName());
			}
		}
		
		UE_LOG(LogTemp, Log, TEXT("[GEManager] 등록 누락 검사 완료 - 누락 없음"));
	}
}

void UNF_GEManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	const UNF_GEManagerSettings* Settings = GetDefault<UNF_GEManagerSettings>();
	
	checkf(IsValid(Settings), TEXT("[GEManager] GE Manager Settings가 유효하지 않습니다."));
	checkf(!Settings->RootGEDataAsset.IsNull(), TEXT("[GEManager] Root GE DataAsset이 설정되지 않았습니다."));
	
	LoadedRootGEDataAsset = Settings->RootGEDataAsset.LoadSynchronous();
	
	checkf(IsValid(LoadedRootGEDataAsset), TEXT("[GEManager] Root GE DataAsset 로드에 실패했습니다."));
	
	UE_LOG(LogTemp, Log, TEXT("[GEManager] Root GE DataAsset 로드: %s"), *GetNameSafe(LoadedRootGEDataAsset));
	
	ValidateGEDataAssets(LoadedRootGEDataAsset.Get());
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