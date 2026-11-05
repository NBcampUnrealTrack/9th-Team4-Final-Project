#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NF_PlayerGEDataAsset.generated.h"

class UGameplayEffect;

UCLASS(BlueprintType)
class PROJECT_NF_API UNF_PlayerGEDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, Category = "GameplayEffects|Player")
	TSubclassOf<UGameplayEffect> NF_SetDamage_GEClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "GameplayEffects|Player")
	TSubclassOf<UGameplayEffect> NF_IncomingExp_GEClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "GameplayEffects|Player")
	TSubclassOf<UGameplayEffect> NF_ShieldRecovery_GEClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "GameplayEffects|Player")
	TSubclassOf<UGameplayEffect> NF_ShieldRecoveryBlocked_GEClass;
};
