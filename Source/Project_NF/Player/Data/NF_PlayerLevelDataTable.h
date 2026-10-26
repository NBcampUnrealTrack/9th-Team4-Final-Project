#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "NF_PlayerLevelDataTable.generated.h"

USTRUCT(BlueprintType)
struct FNF_PlayerLevelDataTable : public FTableRowBase 
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float TargetExp = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MaxCost = 0;
};
