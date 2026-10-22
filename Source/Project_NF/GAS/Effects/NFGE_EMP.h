#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "NFGE_EMP.generated.h"

/** Independent duration effects stack by tag count: removing one EMP does not unlock another. */
UCLASS(Blueprintable)
class PROJECT_NF_API UNFGE_EMP : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UNFGE_EMP();
};
