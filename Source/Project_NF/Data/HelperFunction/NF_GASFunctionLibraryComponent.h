#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "NF_GASFunctionLibraryComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECT_NF_API UNF_GASFunctionLibraryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNF_GASFunctionLibraryComponent();

	UFUNCTION(BlueprintCallable, NetMulticast, Reliable, meta = (AutoCreateRefTerm = "GameplayEventTag"))
	void NetMultiRPC_SendGameplayEventToActor(AActor* TargetActor, const FGameplayTag& GameplayEventTag,
											float PayloadMagnitudeValue = 0.0f, FGameplayEventData PayloadData = FGameplayEventData());
	
	UFUNCTION(BlueprintCallable, Server, Reliable, meta = (AutoCreateRefTerm = "GameplayEventTag"))
	void ServerRPC_SendGameplayEventToActor(AActor* TargetActor, const FGameplayTag& GameplayEventTag,
											float PayloadMagnitudeValue = 0.0f, FGameplayEventData PayloadData = FGameplayEventData());
	
	UFUNCTION(BlueprintCallable, Client, Reliable, meta = (AutoCreateRefTerm = "GameplayEventTag"))
	void ClientRPC_SendGameplayEventToActor(AActor* TargetActor, const FGameplayTag& GameplayEventTag,
											float PayloadMagnitudeValue = 0.0f, FGameplayEventData PayloadData = FGameplayEventData());

};
