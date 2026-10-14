#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NF_InteractionInterface.generated.h"

class APlayerController;

UINTERFACE()
class UNF_InteractionInterface : public UInterface
{
	GENERATED_BODY()
};

class PROJECT_NF_API INF_InteractionInterface
{
	GENERATED_BODY()

public:
	virtual void NF_Interaction(const APlayerController* InteractedController) = 0;
};
