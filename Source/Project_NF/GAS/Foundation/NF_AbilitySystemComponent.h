#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "NF_AbilitySystemComponent.generated.h"


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

private:
	UFUNCTION(Server, Reliable)
	void ServerActivateSkillWithTarget(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityTargetDataHandle& TargetData);
	bool ActivateSkillWithTarget(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityTargetDataHandle& TargetData);

protected:
	virtual void BeginPlay() override;

public:
};
