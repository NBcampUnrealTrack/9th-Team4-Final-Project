#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/Passive/Attack/NFGA_WeaponPassive.h"
#include "NFGA_MagicBullet.generated.h"

// @Owner: 이정준 (구현: Codex) | 2026-10-26
UCLASS(Blueprintable)
class PROJECT_NF_API UNFGA_MagicBullet : public UNFGA_WeaponPassive
{
	GENERATED_BODY()
public:
	UNFGA_MagicBullet();
protected:
	virtual float GetStatMagnitude() const override { return LastRoundDamageMultiplier; }
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Magic Bullet", meta = (ClampMin = "1.0"))
	float LastRoundDamageMultiplier = 2.0f;
};
