#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NF_BossWarningMarker.generated.h"

class UTextRenderComponent;

/** Lightweight local marker used for the targeted player's head warning cue. */
UCLASS(NotBlueprintable)
class PROJECT_NF_API ANF_BossWarningMarker : public AActor
{
	GENERATED_BODY()

public:
	ANF_BossWarningMarker();

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> WarningText;
};
