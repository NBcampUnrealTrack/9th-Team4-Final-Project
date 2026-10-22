#pragma once
#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "NFGE_WeaponPassives.generated.h"

// @Owner: 이정준 (구현: Codex) | 2026-10-21
UCLASS(Blueprintable)
class PROJECT_NF_API UNFGE_AttackDamageBonus : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UNFGE_AttackDamageBonus();
};
UCLASS(Blueprintable)
class PROJECT_NF_API UNFGE_MagicBullet : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UNFGE_MagicBullet();
};
