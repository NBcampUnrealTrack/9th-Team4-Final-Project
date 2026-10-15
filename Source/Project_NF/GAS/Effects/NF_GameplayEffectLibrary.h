#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "NF_GameplayEffectLibrary.generated.h"

class AActor;
class UGameplayEffect;
struct FGameplayEffectSpecHandle;
struct FActiveGameplayEffectHandle;

UCLASS()
class PROJECT_NF_API UNF_GameplayEffectLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "GameplayEffectLibrary")
	static bool NF_ApplyEffectSpecToActor(
		const FGameplayEffectSpecHandle& SpecHandle,
		AActor* TargetActor,
		FActiveGameplayEffectHandle& OutActiveHandle,
		FVector Origin = FVector::ZeroVector,
		const FHitResult& Hit = FHitResult(),
		bool bUseOrigin = false,
		bool bUseHit = false);	
	
	UFUNCTION(BlueprintCallable, Category = "GameplayEffectLibrary")
	static bool NF_RemoveActiveEffectFromActor(
		AActor* TargetActor, 
		FActiveGameplayEffectHandle ActiveHandle);
	
	UFUNCTION(BlueprintCallable, Category = "GameplayEffectLibrary")
	static bool NF_HasActiveEffect(
		AActor* TargetActor,
		TSubclassOf<UGameplayEffect> EffectClass);
	
	UFUNCTION(BlueprintCallable, Category = "GameplayEffectLibrary")
	static int32 NF_RemoveActiveEffectsWithTags(
		AActor* TargetActor,
		const FGameplayTagContainer& Tags);
	
	UFUNCTION(BlueprintCallable, Category = "GameplayEffectLibrary")
	static bool NF_SetSetByCallerMagnitude(
		UPARAM(ref) FGameplayEffectSpecHandle& SpecHandle,
		FGameplayTag DataTag,
		float Magnitude);
	
	UFUNCTION(BlueprintCallable, Category = "GameplayEffectLibrary")
	static int32 NF_GetActiveEffectStackCount(
		AActor* TargetActor,
		TSubclassOf<UGameplayEffect> EffectClass);
	
	UFUNCTION(BlueprintCallable, Category = "GameplayEffectLibrary")
	static float NF_GetActiveEffectRemainingTime(
		AActor* TargetActor,
		TSubclassOf<UGameplayEffect> EffectClass);
};
