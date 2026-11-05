#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NF_GASFunctionLibrary.generated.h"

class UAbilityTask_PlayMontageAndWait;

UCLASS()
class PROJECT_NF_API UNF_GASFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static bool NF_InitAttributeSetDefaultsFromCurveTable(UAbilitySystemComponent* ASC, FName GroupName, int32 Level = 1, bool bFirstInit = true);
	
	static void NF_PlayMontageAndWaitWithEndAbility(UGameplayAbility* OwningAbility, UAnimMontage* MontageToPlay, FName StartSection = NAME_None);
	
	static UAbilityTask_PlayMontageAndWait* NF_PlayMontageAndWait(UGameplayAbility* OwningAbility, UAnimMontage* MontageToPlay,
	bool bStopWhenAbilityEnds = true, FName StartSection = NAME_None);
	
	UFUNCTION(BlueprintCallable, Category = "NF_GASFunctionLibrary", meta = (DefaultToSelf = "TargetActor", AutoCreateRefTerm = "AbilityTagToUse"))
	static void NF_TryAbility(AActor* TargetActor, const FGameplayTag& AbilityTagToUse);

	UFUNCTION(BlueprintCallable, Category = "NF_GASFunctionLibrary", meta = (DefaultToSelf = "WorldContext", AutoCreateRefTerm = "GameplayEventTag"))
	static bool NF_SendGameplayEventTagToActorWithPayloadData(UObject* WorldContext, const FGameplayTag& GameplayEventTag,
		float PayloadMagnitudeValue = 0.0f, FGameplayEventData PayloadData = FGameplayEventData(), AActor* TargetActor = nullptr);
	
	
	UFUNCTION(BlueprintCallable, Category = "NF_GASFunctionLibrary", meta = (AutoCreateRefTerm = "GameplayTag"))
	static bool NF_AddLooseGameplayTagToActor(AActor* TargetActor, const FGameplayTag& GameplayTag, bool bShouldReplicate = false);

	UFUNCTION(BlueprintCallable, Category = "NF_GASFunctionLibrary", meta = (AutoCreateRefTerm = "GameplayTag"))
	static bool NF_RemoveLooseGameplayTagFromActor(AActor* TargetActor, const FGameplayTag& GameplayTag, bool bShouldReplicate = false);
	
	
	UFUNCTION(BlueprintCallable, Category = "NF_GASFunctionLibrary", meta = (AutoCreateRefTerm = "GameplayTagContainer"))
	static bool NF_AddLooseGameplayTagContainerToActor(AActor* TargetActor, const FGameplayTagContainer& GameplayTagContainer, bool bShouldReplicate = false);

	UFUNCTION(BlueprintCallable, Category = "NF_GASFunctionLibrary", meta = (AutoCreateRefTerm = "GameplayTagContainer"))
	static bool NF_RemoveLooseGameplayTagContainerFromActor(AActor* TargetActor, const FGameplayTagContainer& GameplayTagContainer, bool bShouldReplicate = false);
	
	UFUNCTION(BlueprintCallable, Category = "NF_GASFunctionLibrary")static FActiveGameplayEffectHandle NF_ApplyGameplayEffectToSelfWithSetByCaller(
		UAbilitySystemComponent* AbilitySystemComponent,TSubclassOf<UGameplayEffect> GameplayEffectClass, float Magnitude, 
		FGameplayTag DataTag, float Level = 1.0f);
	
	UFUNCTION(BlueprintCallable, Category = "NF_GASFunctionLibrary")
	static FActiveGameplayEffectHandle NF_ApplyGameplayEffectToTargetWithSetByCaller(UAbilitySystemComponent* SourceAbilitySystemComponent,
		AActor* TargetActor, TSubclassOf<UGameplayEffect> GameplayEffectClass, float Magnitude, FGameplayTag DataTag, float Level = 1.0f);

private:
	static AActor* CastFromContext(UObject* ContextObject);
	
};

