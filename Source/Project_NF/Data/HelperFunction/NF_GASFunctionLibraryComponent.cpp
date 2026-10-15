#include "NF_GASFunctionLibraryComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "NF_GASFunctionLibrary.h"
#include "GameFramework/PlayerController.h"


UNF_GASFunctionLibraryComponent::UNF_GASFunctionLibraryComponent()
{
	SetIsReplicatedByDefault(true);
	
}

void UNF_GASFunctionLibraryComponent::ClientRPC_SendGameplayEventToActor_Implementation(AActor* TargetActor,
	const FGameplayTag& GameplayEventTag, float PayloadMagnitudeValue, FGameplayEventData PayloadData)
{
	UNF_GASFunctionLibrary::NF_SendGameplayEventTagToActorWithPayloadData(GetOwner(), GameplayEventTag, PayloadMagnitudeValue,
		PayloadData, TargetActor); 
}

void UNF_GASFunctionLibraryComponent::NetMultiRPC_SendGameplayEventToActor_Implementation(AActor* TargetActor,
                                                                                        const FGameplayTag& GameplayEventTag, float PayloadMagnitudeValue, FGameplayEventData PayloadData)
{
	UNF_GASFunctionLibrary::NF_SendGameplayEventTagToActorWithPayloadData(GetOwner(), GameplayEventTag, PayloadMagnitudeValue,
		PayloadData, TargetActor); 
}


void UNF_GASFunctionLibraryComponent::ServerRPC_SendGameplayEventToActor_Implementation(AActor* TargetActor, const FGameplayTag& GameplayEventTag,
                                                                                      float PayloadMagnitudeValue, FGameplayEventData PayloadData)
{
	UNF_GASFunctionLibrary::NF_SendGameplayEventTagToActorWithPayloadData(GetOwner(), GameplayEventTag, PayloadMagnitudeValue,
		PayloadData, TargetActor); 
}
