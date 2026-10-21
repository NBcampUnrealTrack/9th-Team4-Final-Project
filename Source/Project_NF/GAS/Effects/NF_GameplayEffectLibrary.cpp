#include "NF_GameplayEffectLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

bool UNF_GameplayEffectLibrary::NF_ApplyEffectSpecToActor(const FGameplayEffectSpecHandle& SpecHandle, AActor* TargetActor, FActiveGameplayEffectHandle& OutActiveHandle, FVector Origin, const FHitResult& Hit, bool bUseOrigin, bool bUseHit)
{
	OutActiveHandle = FActiveGameplayEffectHandle();
	
	if (!SpecHandle.IsValid() || !IsValid(TargetActor))
	{
		return false;
	}
	
	UAbilitySystemComponent* TargetASC = 
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	
	if (!IsValid(TargetASC))
	{
		return false;
	}
	
	if (!TargetASC->IsOwnerActorAuthoritative())
	{
		return false;
	}
	
	// Target에 적용할 Spec 복제
	FGameplayEffectSpec TargetSpec(*SpecHandle.Data.Get());
	
	FGameplayEffectContextHandle Context = TargetSpec.GetContext().Duplicate();
	
	if (bUseOrigin)
	{
		Context.AddOrigin(Origin);
	}
	
	if (bUseHit)
	{
		Context.AddHitResult(Hit, true);
	}
	
	TargetSpec.SetContext(Context);
	
	OutActiveHandle = TargetASC->ApplyGameplayEffectSpecToSelf(TargetSpec);
	
	return OutActiveHandle.WasSuccessfullyApplied();
}

bool UNF_GameplayEffectLibrary::NF_RemoveActiveEffectFromActor(
	AActor* TargetActor, 
	FActiveGameplayEffectHandle ActiveHandle)
{
	if (!IsValid(TargetActor) || !ActiveHandle.IsValid())
	{
		return false;
	}
	
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	
	if (!IsValid(TargetASC))
	{
		return false;
	}
	
	if (!TargetASC->IsOwnerActorAuthoritative())
	{
		return false;
	}
	
	return TargetASC->RemoveActiveGameplayEffect(ActiveHandle);
}

bool UNF_GameplayEffectLibrary::NF_HasActiveEffect(
	AActor* TargetActor, 
	TSubclassOf<UGameplayEffect> EffectClass)
{
	if (!IsValid(TargetActor) || !EffectClass)
	{
		return false;
	}
	
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	
	if (!IsValid(TargetASC))
	{
		return false;
	}
	
	TArray<FActiveGameplayEffectHandle> ActiveHandles =
		TargetASC->GetActiveEffects(FGameplayEffectQuery());
	
	for (const FActiveGameplayEffectHandle& Handle : ActiveHandles)
	{
		const FActiveGameplayEffect* ActiveEffect =
			TargetASC->GetActiveGameplayEffect(Handle);
		
		if (ActiveEffect && ActiveEffect->Spec.Def == EffectClass.GetDefaultObject())
		{
			return true;
		}
	}
	
	return false;
}

int32 UNF_GameplayEffectLibrary::NF_RemoveActiveEffectsWithTags(AActor* TargetActor, const FGameplayTagContainer& Tags)
{
	if (!IsValid(TargetActor) || Tags.IsEmpty())
	{
		return 0;
	}
	
	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	
	if (!IsValid(TargetASC))
	{
		return 0;
	}
	
	if (!TargetASC->IsOwnerActorAuthoritative())
	{
		return 0;
	}
	
	return TargetASC->RemoveActiveEffectsWithGrantedTags(Tags);
}

bool UNF_GameplayEffectLibrary::NF_SetSetByCallerMagnitude(FGameplayEffectSpecHandle& SpecHandle, FGameplayTag DataTag, float Magnitude)
{
	if (!SpecHandle.IsValid() || !DataTag.IsValid())
	{
		return false;
	}
	
	if (!FMath::IsFinite(Magnitude))
	{
		return false;
	}
	
	SpecHandle.Data->SetSetByCallerMagnitude(DataTag, Magnitude);
	UE_LOG(LogTemp, Warning, TEXT("SetByCaller Saved: %.1f"),
		SpecHandle.Data->GetSetByCallerMagnitude(DataTag, false, -1.0f));
	
	return true;
}

int32 UNF_GameplayEffectLibrary::NF_GetActiveEffectStackCount(AActor* TargetActor, TSubclassOf<UGameplayEffect> EffectClass)
{
	if (!IsValid(TargetActor) || !EffectClass)
	{
		return 0;
	}
	
	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	
	if (!IsValid(TargetASC))
	{
		return 0;
	}
	
	TArray<FActiveGameplayEffectHandle> ActiveHandles =
		TargetASC->GetActiveEffects(FGameplayEffectQuery());
	
	int32 TotalStackCount = 0;
	
	for (const FActiveGameplayEffectHandle& Handle : ActiveHandles)
	{
		const FActiveGameplayEffect* ActiveEffect =
			TargetASC->GetActiveGameplayEffect(Handle);
		
		if (ActiveEffect && ActiveEffect->Spec.Def == EffectClass.GetDefaultObject())
		{
			TotalStackCount += TargetASC->GetCurrentStackCount(Handle);
		}
	}
	
	return TotalStackCount;
}

float UNF_GameplayEffectLibrary::NF_GetActiveEffectRemainingTime(AActor* TargetActor, TSubclassOf<UGameplayEffect> EffectClass)
{
	if (!IsValid(TargetActor) || !EffectClass)
	{
		return 0.0f;
	}
	
	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	
	if (!IsValid(TargetASC) || !IsValid(TargetASC->GetWorld()))
	{
		return 0.0f;
	}
	
	TArray<FActiveGameplayEffectHandle> ActiveHandles =
		TargetASC->GetActiveEffects(FGameplayEffectQuery());
	
	float MaxRemainingTime = 0.0f;
	
	for (const FActiveGameplayEffectHandle& Handle : ActiveHandles)
	{
		const FActiveGameplayEffect* ActiveEffect =
			TargetASC->GetActiveGameplayEffect(Handle);
		
		if (!ActiveEffect || ActiveEffect->Spec.Def != EffectClass.GetDefaultObject())
		{
			continue;
		}
		
		const float RemainingTime = 
			ActiveEffect->GetTimeRemaining(TargetASC->GetWorld()->GetTimeSeconds());
		
		if (RemainingTime < 0.0f)
		{
			return -1.0f;
		}
		
		MaxRemainingTime = FMath::Max(MaxRemainingTime, RemainingTime);
	}
	
	return MaxRemainingTime;
}
