#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NF_GASFunctionLibrary.generated.h"

UCLASS()
class PROJECT_NF_API UNF_GASFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static bool NF_InitAttributeSetDefaultsFromCurveTable(UAbilitySystemComponent* ASC, FName GroupName, int32 Level = 1, bool bFirstInit = true);
	
	UFUNCTION(BlueprintCallable, Category = "NF_GASFunctionLibrary", meta = (DefaultToSelf = "TargetActor", AutoCreateRefTerm = "AbilityTagToUse"))
	static void NF_TryAbility(AActor* TargetActor, const FGameplayTag& AbilityTagToUse);

	UFUNCTION(BlueprintCallable, Category = "NF_GASFunctionLibrary", meta = (DefaultToSelf = "WorldContext", AutoCreateRefTerm = "GameplayEventTag"))
	static void NF_SendGameplayEventTagToActorWithPayloadData(UObject* WorldContext, const FGameplayTag& GameplayEventTag,
		float PayloadMagnitudeValue = 0.0f, FGameplayEventData PayloadData = FGameplayEventData(), AActor* DearActor = nullptr);
	
	
	UFUNCTION(BlueprintCallable, Category = "NF_GASFunctionLibrary", meta = (AutoCreateRefTerm = "GameplayTag"))
	static bool NF_AddLooseGameplayTagToActor(AActor* TargetActor, const FGameplayTag& GameplayTag, bool bShouldReplicate = false);

	UFUNCTION(BlueprintCallable, Category = "NF_GASFunctionLibrary", meta = (AutoCreateRefTerm = "GameplayTag"))
	static bool NF_RemoveLooseGameplayTagFromActor(AActor* TargetActor, const FGameplayTag& GameplayTag, bool bShouldReplicate = false);
	
	
	UFUNCTION(BlueprintCallable, Category = "NF_GASFunctionLibrary", meta = (AutoCreateRefTerm = "GameplayTagContainer"))
	static bool NF_AddLooseGameplayTagContainerToActor(AActor* TargetActor, const FGameplayTagContainer& GameplayTagContainer, bool bShouldReplicate = false);

	UFUNCTION(BlueprintCallable, Category = "NF_GASFunctionLibrary", meta = (AutoCreateRefTerm = "GameplayTagContainer"))
	static bool NF_RemoveLooseGameplayTagContainerFromActor(AActor* TargetActor, const FGameplayTagContainer& GameplayTagContainer, bool bShouldReplicate = false);
	
	
private:
	static AActor* CastFromContext(UObject* ContextObject);
	
};

