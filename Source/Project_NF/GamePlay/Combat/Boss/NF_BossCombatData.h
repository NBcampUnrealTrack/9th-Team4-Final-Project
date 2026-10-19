#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "NF_BossCombatData.generated.h"

UENUM(BlueprintType)
enum class ENF_BossPatternId : uint8
{
	AreaVolley,
	ThreatVolley,
	GroundDamageZone,
	Relocate,
	CenterBlast,
	LastGasp
};

/** One row in the boss combat tuning DataTable. Damage values are per hit/tick. */
USTRUCT(BlueprintType)
struct PROJECT_NF_API FNF_BossCombatDataRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	float MaxHealth = 10000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Combat|Damage")
	float AreaVolleyDamage = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Combat|Damage")
	float ThreatVolleyDamage = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Combat|Damage")
	float GroundDamageZoneDamagePerTick = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Combat|Damage")
	float RelocateDamage = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Combat|Damage")
	float CenterBlastDamage = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Combat|Damage")
	float LastGaspDamage = 100.0f;
};
