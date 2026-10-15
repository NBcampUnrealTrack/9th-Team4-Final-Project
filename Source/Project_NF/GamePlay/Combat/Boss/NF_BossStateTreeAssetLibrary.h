#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NF_BossStateTreeAssetLibrary.generated.h"

class UStateTree;

/** Editor helper for creating the minimal StateTree asset used by ANF_BossBase. */
UCLASS()
class PROJECT_NF_API UNF_BossStateTreeAssetLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "NF|Boss|Editor", meta = (DevelopmentOnly))
	static UStateTree* CreateBossPatternCycleStateTree();

	static bool AssignBossPatternCycleToTestBoss(UStateTree* StateTree);
};
