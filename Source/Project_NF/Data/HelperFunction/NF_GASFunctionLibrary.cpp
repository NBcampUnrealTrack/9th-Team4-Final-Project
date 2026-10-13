
#include "NF_GASFunctionLibrary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Kismet/KismetSystemLibrary.h"


void UNF_GASFunctionLibrary::NF_TryAbility(AActor* TargetActor, const FGameplayTag& AbilityTagToUse)
{
	if (IsValid(TargetActor) == false)
	{
		UKismetSystemLibrary::PrintString(TargetActor,
			TEXT("[NF_TryAbility]함수의 [TargetActor]가 설정되지않았습니다. Ability 실행 주체를 설정하세요."),
				true, true, FColor::Magenta, 10.0f);
		
		return;
	}
	
	if (AbilityTagToUse.IsValid() == false)
	{
		UKismetSystemLibrary::PrintString(TargetActor,
		                                  (TEXT("[NF_TryAbility]함수의 Tag가 채워지지않아, [GameplayAbility]가 동작하지않습니다.")),
		                                  true, true, FColor::Magenta, 10.0f);
		return;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (IsValid(ASC) == false) return;

}


void UNF_GASFunctionLibrary::NF_SendGameplayEventTagToActorWithPayloadData(UObject* WorldContext,
	const FGameplayTag& GameplayEventTag, float PayloadMagnitudeValue, FGameplayEventData PayloadData, AActor* DearActor)
{
	if (GameplayEventTag.IsValid() == false) return;
	
	AActor* ContextActor = CastFromContext(WorldContext);
	if (IsValid(ContextActor) == false) return;
	
	if (IsValid(DearActor) == false)
	{
		DearActor = ContextActor;
	}
	
	FGameplayEventData CurrentEventData = PayloadData;
	CurrentEventData.EventTag = GameplayEventTag;
	CurrentEventData.EventMagnitude = PayloadMagnitudeValue;
	
	if (IsValid(CurrentEventData.Instigator) == false) 
	{
		CurrentEventData.Instigator = ContextActor;
	}
	
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(DearActor, GameplayEventTag, CurrentEventData);
}


AActor* UNF_GASFunctionLibrary::CastFromContext(UObject* ContextObject)
{
	AActor* Actor = Cast<AActor>(ContextObject);
	if (IsValid(Actor) == true) return Actor;
	
	UGameplayAbility* Ability = Cast<UGameplayAbility>(ContextObject);
	if (IsValid(Ability) == true) return Ability->GetAvatarActorFromActorInfo();

	UActorComponent* Component = Cast<UActorComponent>(ContextObject);
	if (IsValid(Component) == true) return Component->GetOwner();
	
	UAnimInstance* AnimInstance = Cast<UAnimInstance>(ContextObject);
	if (IsValid(AnimInstance) == true) return AnimInstance->GetOwningActor();
	
	return nullptr;
}


bool UNF_GASFunctionLibrary::NF_InitAttributeSetDefaultsFromCurveTable(UAbilitySystemComponent* AbilitySystemComponent, FName GroupName, int32 Level, bool bFirstInit)
{
	if (IsValid(AbilitySystemComponent) == false) return false;
	if (AbilitySystemComponent->IsOwnerActorAuthoritative() == false) return false;

	FAttributeSetInitter* AttributeSetInitter = UAbilitySystemGlobals::Get().GetAttributeSetInitter();
	if (AttributeSetInitter == nullptr) return false;

	if (Level < 1)
	{
		Level = 1;
	}

	AttributeSetInitter->InitAttributeSetDefaults(AbilitySystemComponent, GroupName, Level, bFirstInit);
	return true;
}


bool UNF_GASFunctionLibrary::NF_AddLooseGameplayTagToActor(AActor* TargetActor, const FGameplayTag& GameplayTag, bool bShouldReplicate)
{
	if (IsValid(TargetActor) == false) return false;
	if (GameplayTag.IsValid() == false) return false;
	if (bShouldReplicate == true && TargetActor->HasAuthority() == false) return false;

	FGameplayTagContainer GameplayTags;
	GameplayTags.AddTag(GameplayTag);

	return UAbilitySystemBlueprintLibrary::AddLooseGameplayTags(TargetActor, GameplayTags, bShouldReplicate);
}


bool UNF_GASFunctionLibrary::NF_RemoveLooseGameplayTagFromActor(AActor* TargetActor, const FGameplayTag& GameplayTag, bool bShouldReplicate)
{
	if (IsValid(TargetActor) == false) return false;
	if (GameplayTag.IsValid() == false) return false;
	if (bShouldReplicate == true && TargetActor->HasAuthority() == false) return false;
	

	FGameplayTagContainer GameplayTags;
	GameplayTags.AddTag(GameplayTag);

	return UAbilitySystemBlueprintLibrary::RemoveLooseGameplayTags(TargetActor, GameplayTags, bShouldReplicate);
}


bool UNF_GASFunctionLibrary::NF_AddLooseGameplayTagContainerToActor(AActor* TargetActor, const FGameplayTagContainer& GameplayTagContainer, bool bShouldReplicate)
{
	if (IsValid(TargetActor) == false) return false;
	if (GameplayTagContainer.IsEmpty() == true) return false;
	if (bShouldReplicate == true && TargetActor->HasAuthority() == false) return false;
	

	return UAbilitySystemBlueprintLibrary::AddLooseGameplayTags(TargetActor, GameplayTagContainer, bShouldReplicate);
}


bool UNF_GASFunctionLibrary::NF_RemoveLooseGameplayTagContainerFromActor(AActor* TargetActor, const FGameplayTagContainer& GameplayTagContainer, bool bShouldReplicate)
{
	if (IsValid(TargetActor) == false) return false;
	if (GameplayTagContainer.IsEmpty() == true) return false;
	if (bShouldReplicate == true && TargetActor->HasAuthority() == false) return false;
	

	return UAbilitySystemBlueprintLibrary::RemoveLooseGameplayTags(TargetActor, GameplayTagContainer, bShouldReplicate);
}


