#pragma once

#include "CoreMinimal.h"
#include "NF_BuildTypes.generated.h"

UENUM(BlueprintType)
enum class ENF_BuildItemType : uint8
{
	Weapon,
	Armor,
	ActiveSkill,
	PassiveSkill
};

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
	GrantFailed
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
