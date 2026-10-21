#pragma once
#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "NFGE_DoubleJump.generated.h"

// @Owner: 이정준 (구현: Codex) | 2026-10-21
UCLASS(Blueprintable)
class PROJECT_NF_API UNFGE_DoubleJump : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UNFGE_DoubleJump();
};
