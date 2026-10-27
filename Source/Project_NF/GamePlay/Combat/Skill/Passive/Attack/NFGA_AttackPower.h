#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/Passive/Attack/NFGA_WeaponPassive.h"
#include "NFGA_AttackPower.generated.h"

// @Owner: 이정준 (구현: Codex) | 2026-10-26
UCLASS(Blueprintable)
class PROJECT_NF_API UNFGA_AttackPower : public UNFGA_WeaponPassive
{
	GENERATED_BODY()
public:
	UNFGA_AttackPower();
protected:
	virtual float GetStatMagnitude() const override { return AttackDamageBonus; }
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Attack Power", meta = (ClampMin = "0.0"))
	float AttackDamageBonus = 10.0f;
};
