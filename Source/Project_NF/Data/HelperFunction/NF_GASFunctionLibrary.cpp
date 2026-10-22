
#include "NF_GASFunctionLibrary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
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
	
	ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(AbilityTagToUse)); 

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

void UNF_GASFunctionLibrary::NF_PlayMontageAndWaitWithEndAbility(UGameplayAbility* OwningAbility,
	UAnimMontage* MontageToPlay, FName StartSection)
{
	if (IsValid(OwningAbility) == false) return;

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		OwningAbility,
		NAME_None,
		MontageToPlay,
		1.0f,
		StartSection,    // [변경] NAME_None 고정 → 매개변수로 전달
		true,
		1.0f,
		0.0f,
		false);
	
	// UGameplayAbility의 K2_EndAbility는 protected라서 &UGameplayAbility::K2_EndAbility를 쓸 수 없어 이름으로 바인딩
	FScriptDelegate EndAbilityDelegate;
	EndAbilityDelegate.BindUFunction(OwningAbility, TEXT("K2_EndAbility"));

	MontageTask->OnCompleted.AddUnique(EndAbilityDelegate);
	MontageTask->OnInterrupted.AddUnique(EndAbilityDelegate);
	MontageTask->OnCancelled.AddUnique(EndAbilityDelegate);

	MontageTask->ReadyForActivation();
}

UAbilityTask_PlayMontageAndWait* UNF_GASFunctionLibrary::NF_PlayMontageAndWait(UGameplayAbility* OwningAbility,
	UAnimMontage* MontageToPlay, bool bStopWhenAbilityEnds, FName StartSection)
{
	if (IsValid(OwningAbility) == false) return nullptr;   // Task를 소유할 GA가 없으면 생성 불가
	if (IsValid(MontageToPlay) == false) return nullptr;   // 재생할 몽타주가 없으면 생성 불가
	
	// 기존 헬퍼와 같은 설정으로 Task만 생성. EndAbility 연결 X, 재생 시작(ReadyForActivation) X
	return UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		OwningAbility,
		NAME_None,
		MontageToPlay,
		1.0f,
		StartSection,
		bStopWhenAbilityEnds,
		1.0f,
		0.0f,
		false);
}


