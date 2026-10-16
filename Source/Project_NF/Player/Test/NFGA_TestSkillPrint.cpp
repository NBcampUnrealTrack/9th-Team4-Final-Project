#include "NFGA_TestSkillPrint.h"

#include "AbilitySystemComponent.h"
#include "Kismet/KismetSystemLibrary.h"


UNFGA_TestSkillPrint::UNFGA_TestSkillPrint()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}


void UNFGA_TestSkillPrint::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	int32 InputID = INDEX_NONE;
	const FGameplayAbilitySpec* CurrentSpec = GetCurrentAbilitySpec();
	if (CurrentSpec != nullptr)
	{
		InputID = CurrentSpec->InputID;
	}
	
	FString NetRoleText = TEXT("Client");
	if (HasAuthority(&ActivationInfo) == true)
	{
		NetRoleText = TEXT("Server");
	}
	
	const FString Message = FString::Printf(TEXT("[%s] %s (InputID %d)"), *NetRoleText, *PrintMessage, InputID);
	UKismetSystemLibrary::PrintString(GetAvatarActorFromActorInfo(), Message, true, true, FColor::Cyan, 3.0f);
	
	// 캐릭터 입력은 Pressed만 보내므로 바로 종료해야 다음 입력 때 다시 발동됨
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
