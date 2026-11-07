#pragma once

#include "CoreMinimal.h"
#include "Data/Enum/NF_ItemTypes.h"
#include "GamePlay/Combat/Armor/NF_ArmorTypes.h"
#include "Engine/DataAsset.h"
#include "UObject/Interface.h"
#include "NF_BuildTypesInterface.generated.h"

class UNF_BuildRules;
class UNF_BuildItemDefinitionDataAsset;

UENUM(BlueprintType)
enum class ENF_BuildResult : uint8
{
	Success,
	NotAuthority,
	NotInBuildZone,
	BuildBlocked,
	MissingAbilitySystem,
	InvalidRules,
	UnknownItem,
	DuplicateItem,
	InvalidSlot,
	RequiredWeaponMissing,
	CostLimitExceeded,
	InvalidGrant,
	GrantFailed,
	ItemNotUnlocked
};

// @Owner: 황준현 | 2026-10-15
// 플레이어의 빌드 상점 범위 진입·이탈 상태.
UENUM(BlueprintType)
enum class ENF_BuildShopRangeState : uint8
{
	Outside UMETA(DisplayName = "상점 범위 밖"),
	Inside UMETA(DisplayName = "상점 범위 안")
};

// @Owner: 황준현 | 2026-10-15
// 로컬 플레이어의 빌드 상점 UI 표시 상태.
UENUM(BlueprintType)
enum class ENF_BuildShopUIState : uint8
{
	None UMETA(DisplayName = "None"),
	BuildOpened UMETA(DisplayName = "빌드창 열림"),
	BuildClosed UMETA(DisplayName = "빌드창 닫힘")
};

/** The client submits identity/slot only. Costs and GAS classes come from server data. */
USTRUCT(BlueprintType)
struct PROJECT_NF_API FNF_BuildSelection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Build")
	FName ItemId;

	/** Zero-based slot within the item's category. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Build", meta = (ClampMin = "0"))
	int32 SlotIndex = 0;
};

USTRUCT(BlueprintType)
struct PROJECT_NF_API FNF_BuildValidation
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "NF|Build")
	ENF_BuildResult Result = ENF_BuildResult::Success;

	UPROPERTY(BlueprintReadOnly, Category = "NF|Build")
	int32 TotalCost = 0;

	UPROPERTY(BlueprintReadOnly, Category = "NF|Build")
	int32 CostLimit = 0;

	UPROPERTY(BlueprintReadOnly, Category = "NF|Build")
	FText Message;
};

class ANF_ArmorBase;
class ANF_WeaponBase;
class UNF_WeaponDataAsset;
class UNF_ArmorDataAsset;
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

	// @Owner: 황준현 | 2026-10-28. Rules가 선택한 원본에서 채우는 티어별 참조.
	UPROPERTY(BlueprintReadOnly, Transient, Category = "NF|Build|Source")
	TObjectPtr<UNF_WeaponDataAsset> WeaponData;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "NF|Build|Source")
	TObjectPtr<UNF_ArmorDataAsset> ArmorData;

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

class UNF_WeaponDataAsset;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UNF_BuildWeaponDataReceiver : public UInterface
{
	GENERATED_BODY()
};

// @Owner: 황준현 | 2026-10-28. 무기 담당자가 구현할 서버 장착 전 초기화 계약.
// 성공 시 GetWeaponData()가 NewData를 반환해야 한다. 탄약/외형 초기화와
// WeaponData 복제 및 클라이언트 갱신은 무기 구현이 담당한다.
class PROJECT_NF_API INF_BuildWeaponDataReceiver
{
	GENERATED_BODY()

public:
	virtual bool ApplyBuildWeaponData(UNF_WeaponDataAsset* NewData) = 0;
};

// 한 번의 조회에서만 사용하는 원본 데이터. 해석 실패 시 Data.Cost는 -1이다.
struct FNF_BuildCatalogItem
{
	UNF_BuildItemDefinitionDataAsset* Definition = nullptr;
	FNF_BuildItemData Data;
	UDataAsset* Source = nullptr;
};

struct FNF_BuildPreviewItem : FNF_BuildCatalogItem
{
	FNF_BuildPreviewItem() = default;
	explicit FNF_BuildPreviewItem(const FNF_BuildCatalogItem& Item) : FNF_BuildCatalogItem(Item) {}

	int32 SlotIndex = INDEX_NONE;
	int32 CostAfterSelection = INDEX_NONE;
};

struct FNF_BuildShopPreview
{
	FNF_BuildValidation Validation;
	int32 DisplayCost = 0;
	TArray<FNF_BuildPreviewItem> Draft;
	TArray<FNF_BuildPreviewItem> Catalog;
};

/** @Owner: 황준현 | 2026-11-01. 장착 항목만 소유 클라이언트에 전달하는 확정 데이터. */
USTRUCT()
struct PROJECT_NF_API FNF_BuildEquippedItem
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UNF_BuildItemDefinitionDataAsset> Definition;

	UPROPERTY()
	TObjectPtr<UDataAsset> Source;

	UPROPERTY()
	FNF_BuildItemData ItemData;
};

// 호출 중에만 사용하는 플레이어 상태 참조. 서브시스템에 저장하지 않는다.
struct FNF_BuildQueryContext
{
	const UNF_BuildRules* Rules;
	int32 CostLimit;
	bool bInitialized;
	const TArray<FNF_BuildSelection>& CurrentBuild;
	const TArray<FNF_BuildSelection>& DraftBuild;
	const TSet<FName>& PickedItemIds;
	const TArray<FNF_BuildEquippedItem>& EquippedItems;
};

enum class ENF_BuildFallbackAction : uint8
{
	Reject,
	KeepCurrent,
	Apply
};
