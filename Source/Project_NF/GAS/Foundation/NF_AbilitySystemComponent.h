#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "NF_AbilitySystemComponent.generated.h"

struct FOnAttributeChangeData;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECT_NF_API UNF_AbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UNF_AbilitySystemComponent();

	// @Owner: 스킬 담당 (구현: Codex) | 2026-10-13
	// ServerOnly 스킬: 소유 클라이언트가 확정한 목표를 서버 검증 후 발동한다.
	UFUNCTION(BlueprintCallable, Category = "NF|Skill")
	bool RequestSkillActivationWithTarget(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityTargetDataHandle& TargetData);
	UFUNCTION(BlueprintCallable, Category = "NF|Skill")
	void ReleaseSkillInput(FGameplayAbilitySpecHandle Handle);

private:
	UFUNCTION(Server, Reliable)
	void ServerReleaseSkillInput(FGameplayAbilitySpecHandle Handle);
	UFUNCTION(Server, Reliable)
	void ServerActivateSkillWithTarget(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityTargetDataHandle& TargetData);
	bool ActivateSkillWithTarget(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityTargetDataHandle& TargetData);
	
	// @Owner: 지우 | 2026-10-23
	void OnGameplayEffectAdded(
		UAbilitySystemComponent* TargetASC,
		const FGameplayEffectSpec& EffectSpec,
		FActiveGameplayEffectHandle ActiveHandle);
	
	void OnCooldownReductionRateChanged(const FOnAttributeChangeData& Data);

protected:
	virtual void BeginPlay() override;

public:
};
