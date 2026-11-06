#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "NF_GEManagerSettings.generated.h"

class UNF_GERootDataAsset;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "GE Manager"))
class PROJECT_NF_API UNF_GEManagerSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	UNF_GEManagerSettings();
	
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Gameplay Effects")
	TSoftObjectPtr<UNF_GERootDataAsset> RootGEDataAsset;
};
