#include "NF_AbilitySystemComponent.h"


#include "GamePlay/Combat/Skill/NFGA_SkillBase.h"
#include "Project_NF.h"

UNF_AbilitySystemComponent::UNF_AbilitySystemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}

void UNF_AbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();

}

bool UNF_AbilitySystemComponent::RequestSkillActivationWithTarget(FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityTargetDataHandle& TargetData)
{
	if (!FindAbilitySpecFromHandle(Handle))
	{
		return false;
	}
	if (IsOwnerActorAuthoritative())
	{
		return ActivateSkillWithTarget(Handle, TargetData);
	}
	if (!AbilityActorInfo.IsValid() || !AbilityActorInfo->IsLocallyControlled())
	{
		return false;
	}
	ServerActivateSkillWithTarget(Handle, TargetData);
	return true; // 요청을 전송했다는 의미. 실제 성공 여부는 서버가 결정한다.
}

void UNF_AbilitySystemComponent::ServerActivateSkillWithTarget_Implementation(FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityTargetDataHandle& TargetData)
{
	FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Handle);
	const UGameplayAbility* Ability = Spec ? Spec->Ability.Get() : nullptr;
	if (!Ability || Ability->GetNetSecurityPolicy() == EGameplayAbilityNetSecurityPolicy::ServerOnly
		|| Ability->GetNetSecurityPolicy() == EGameplayAbilityNetSecurityPolicy::ServerOnlyExecution
		|| !ActivateSkillWithTarget(Handle, TargetData))
	{
		UE_LOG(LogProject_NF, Warning, TEXT("[Skill] Target activation rejected: ownership, policy, target or activation conditions."));
	}
}

bool UNF_AbilitySystemComponent::ActivateSkillWithTarget(FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityTargetDataHandle& TargetData)
{
	FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Handle);
	UNFGA_SkillBase* Skill = Spec ? Cast<UNFGA_SkillBase>(Spec->GetPrimaryInstance()) : nullptr;
	if (!IsOwnerActorAuthoritative() || !Skill || Spec->IsActive()
		|| Skill->GetNetExecutionPolicy() != EGameplayAbilityNetExecutionPolicy::ServerOnly
		|| !Skill->PrepareConfirmedTargetData(TargetData))
	{
		return false;
	}
	const bool bActivated = TryActivateAbility(Handle, false);
	if (!bActivated)
	{
		Skill->ClearConfirmedTargetData();
	}
	return bActivated;
}


