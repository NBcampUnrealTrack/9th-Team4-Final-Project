#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_TestSkillPrint.generated.h"

// 개인 테스트용: 슬롯 버튼으로 발동되면 화면에 글씨만 출력하고 바로 종료
UCLASS()
class PROJECT_NF_API UGA_TestSkillPrint : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_TestSkillPrint();
	
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Test")
	FString PrintMessage = TEXT("테스트 스킬 발동");
};
