#pragma once

#include "CoreMinimal.h"
#include "Data/Enum/NF_ItemTypes.h"
#include "GamePlay/Combat/Armor/NF_ArmorTypes.h"
#include "UObject/Interface.h"
#include "NF_BuildItemSource.generated.h"

class ANF_ArmorBase;
class ANF_WeaponBase;
class UGameplayAbility;
class UGameplayEffect;

/** @Owner: 황준현 | 2026-10-26. 원본에서 조회한 결과. 빌드 DA에 저장하는 설정이 아니다. */
USTRUCT(BlueprintType)
struct PROJECT_NF_API FNF_BuildItemData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Transient, Category = "NF|Build|Source")
	ENF_ItemType ItemType = static_cast<ENF_ItemType>(255);

	// 미연결 원본을 무료 항목으로 취급하지 않는다. 0 코스트는 원본이 명시적으로 반환해야 한다.
	UPROPERTY(BlueprintReadWrite, Transient, Category = "NF|Build|Source")
	int32 Cost = -1;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "NF|Build|Source")
	ENF_ArmorSlot ArmorSlot = ENF_ArmorSlot::None;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "NF|Build|Source")
	TSubclassOf<ANF_WeaponBase> WeaponClass;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "NF|Build|Source")
	TSubclassOf<ANF_ArmorBase> ArmorClass;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "NF|Build|Source")
	TSubclassOf<UGameplayAbility> AbilityClass;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "NF|Build|Source")
	int32 AbilityLevel = 1;

	// GA 자체의 PassiveEffects와 중복 부여하지 않는다. 원본 담당자가 수명을 구분한다.
	UPROPERTY(BlueprintReadWrite, Transient, Category = "NF|Build|Source")
	TArray<TSubclassOf<UGameplayEffect>> EquippedEffects;
};

UINTERFACE(BlueprintType, Blueprintable)
class PROJECT_NF_API UNF_BuildItemSource : public UInterface
{
	GENERATED_BODY()
};

/** 원본 DA 담당자가 구현한다. 서버/클라이언트에서 같은 정적 원본을 부작용 없이 조회해야 한다. */
class PROJECT_NF_API INF_BuildItemSource
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NF|Build|Source")
	bool GetBuildItemData(FNF_BuildItemData& OutData) const;
	virtual bool GetBuildItemData_Implementation(FNF_BuildItemData& OutData) const;
};

/** 빌드 슬롯 정책은 각 파트의 원본 데이터와 분리한다. */
class PROJECT_NF_API FNF_BuildItemSlots
{
public:
	static int32 GetDefaultSlotIndex(const FNF_BuildItemData& Data);
	static bool CanUseSlot(const FNF_BuildItemData& Data, int32 SlotIndex);
};
