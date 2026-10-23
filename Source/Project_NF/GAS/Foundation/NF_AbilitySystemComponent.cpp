#include "NF_AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GAS/Tags/NFTag_Cooldown.h"
#include "GamePlay/Combat/Skill/NFGA_SkillBase.h"
#include "Project_NF.h"
#include "GAS/Attributes/NF_KeyHolderAttributeSet.h"
#include "GameplayEffectTypes.h"
#include "Engine/World.h"

UNF_AbilitySystemComponent::UNF_AbilitySystemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}

void UNF_AbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (!IsOwnerActorAuthoritative())
	{
		return;
	}
	
	OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(this, &UNF_AbilitySystemComponent::OnGameplayEffectAdded);
	
	GetGameplayAttributeValueChangeDelegate(UNF_KeyHolderAttributeSet::GetCooldownReductionRateAttribute()).AddUObject(this, &UNF_AbilitySystemComponent::OnCooldownReductionRateChanged);
	
	// test
	const UNF_KeyHolderAttributeSet* KeyHolderSet = GetSet<UNF_KeyHolderAttributeSet>();
	UE_LOG(LogTemp, Warning, TEXT("[KeyCooldown] KeyHolder AttributeSet: %s"),
		KeyHolderSet ? TEXT("Registered") : TEXT("Missing"));
}

void UNF_AbilitySystemComponent::OnGameplayEffectAdded(UAbilitySystemComponent* TargetASC, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveHandle)
{
	if (!IsOwnerActorAuthoritative() || TargetASC != this || !ActiveHandle.IsValid())
	{
		return;
	}
	
	if (!EffectSpec.Def || EffectSpec.Def->DurationPolicy != EGameplayEffectDurationType::HasDuration || EffectSpec.GetDuration() <= 0.0f)
	{
		return;
	}
	
	FGameplayTagContainer GrantedTags;
	EffectSpec.GetAllGrantedTags(GrantedTags);
	
	if (!GrantedTags.HasTag(NFTag_Cooldown::Cooldown_Skill))
	{
		return;
	}
	
	bool bFoundAttribute = false;
	const float CooldownReductionRate = GetGameplayAttributeValue(
		UNF_KeyHolderAttributeSet::GetCooldownReductionRateAttribute(),
		bFoundAttribute);
	
	if (!bFoundAttribute || CooldownReductionRate <= 0.0f)
	{
		return;
	}
	
	const float ReductionRate = FMath::Clamp(CooldownReductionRate, 0.0f, 1.0f);
	const float BaseCooldown = EffectSpec.GetDuration();
	const float ReductionSeconds = BaseCooldown * ReductionRate;
	
	if (ReductionSeconds >= BaseCooldown)
	{
		RemoveActiveGameplayEffect(ActiveHandle);
		return;
	}
	
	const FString EffectName = GetNameSafe(EffectSpec.Def);
	
	ModifyActiveEffectStartTime(ActiveHandle, -ReductionSeconds);
	UE_LOG(LogTemp, Warning, TEXT("[KeyCooldown] GE: %s, Base: %.2f, Rate: %.2f, Reduced: %.2f"),
		*EffectName, BaseCooldown, ReductionRate, ReductionSeconds);
}

void UNF_AbilitySystemComponent::OnCooldownReductionRateChanged(const FOnAttributeChangeData& Data)
{
	if (!IsOwnerActorAuthoritative())
	{
		return;
	}
	
	if (Data.OldValue > 0.0f || Data.NewValue <= 0.0f)
	{
		return;
	}
	
	const float ReductionRate = FMath::Clamp(Data.NewValue, 0.0f, 1.0f);
	
	FGameplayTagContainer CooldownTags;
	CooldownTags.AddTag(NFTag_Cooldown::Cooldown_Skill);
	
	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(CooldownTags);
	
	const TArray<FActiveGameplayEffectHandle> CooldownHandles = GetActiveEffects(Query);
	
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	
	for (const FActiveGameplayEffectHandle& Handle : CooldownHandles)
	{
		const FActiveGameplayEffect* ActiveEffect = GetActiveGameplayEffect(Handle);
		
		if (!ActiveEffect)
		{
			continue;
		}
		
		if (!ActiveEffect->Spec.Def || ActiveEffect->Spec.Def->DurationPolicy != EGameplayEffectDurationType::HasDuration)
		{
			continue;
		}
		
		const float BaseCooldown = ActiveEffect->Spec.GetDuration();
		
		if (BaseCooldown <= 0.0f)
		{
			continue;
		}
		
		const float RemainingCooldown = ActiveEffect->GetTimeRemaining(World->GetTimeSeconds());
		
		const float ReductionSeconds = BaseCooldown * ReductionRate;
		
		if (RemainingCooldown <= ReductionSeconds)
		{
			RemoveActiveGameplayEffect(Handle);
			continue;
		}
		
		const FString EffectName = GetNameSafe(ActiveEffect->Spec.Def);
		
		ModifyActiveEffectStartTime(Handle, -ReductionSeconds);
		UE_LOG(LogTemp, Warning, TEXT("[KeyCooldown] Existing GE: %s, Remaining: %.2f, Reduced: %.2f"),
			*EffectName, RemainingCooldown, ReductionSeconds);
	}
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

void UNF_AbilitySystemComponent::ReleaseSkillInput(FGameplayAbilitySpecHandle Handle)
{
	if (IsOwnerActorAuthoritative())
	{
		FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Handle);
		if (auto* Skill = Spec ? Cast<UNFGA_SkillBase>(Spec->GetPrimaryInstance()) : nullptr) { Skill->ReleaseSkillInput(); }
	}
	else if (AbilityActorInfo.IsValid() && AbilityActorInfo->IsLocallyControlled()) { ServerReleaseSkillInput(Handle); }
}

void UNF_AbilitySystemComponent::ServerReleaseSkillInput_Implementation(FGameplayAbilitySpecHandle Handle)
{
	FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Handle);
	auto* Skill = Spec ? Cast<UNFGA_SkillBase>(Spec->GetPrimaryInstance()) : nullptr;
	if (Skill && Skill->GetNetSecurityPolicy() != EGameplayAbilityNetSecurityPolicy::ServerOnly
		&& Skill->GetNetSecurityPolicy() != EGameplayAbilityNetSecurityPolicy::ServerOnlyTermination) { Skill->ReleaseSkillInput(); }
}


