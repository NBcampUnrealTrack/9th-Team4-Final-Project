#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NF_MeleeAttackInterface.generated.h"

class APlayerController;

UINTERFACE()
class UNF_MeleeAttackInterface : public UInterface
{
	GENERATED_BODY()
};

class PROJECT_NF_API INF_MeleeAttackInterface
{
	GENERATED_BODY()

public:
	virtual UMeshComponent* NF_GetMeleeAttackMesh() = 0;
};
