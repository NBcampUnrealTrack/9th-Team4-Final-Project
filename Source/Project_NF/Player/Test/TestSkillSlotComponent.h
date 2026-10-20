#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "TestSkillSlotComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;

// 개인 테스트용: NF_BuildComponent의 슬롯 InputID 등록(GrantItem)과 Press/Release 부분만 떼어 온 컴포넌트
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECT_NF_API UTestSkillSlotComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTestSkillSlotComponent();
	
	UFUNCTION(BlueprintCallable, Category = "NF|Test")
	void PressTestSkillInput(int32 SlotIndex);
	
	UFUNCTION(BlueprintCallable, Category = "NF|Test")
	void ReleaseTestSkillInput(int32 SlotIndex);
	
protected:
	virtual void BeginPlay() override;
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	
	// 0번 = InputID 1 (SkillA 버튼), 1번 = InputID 2 (SkillB 버튼)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Test")
	TArray<TSubclassOf<UGameplayAbility>> TestSlotAbilityArray;
	
private:
	void TryGrantTestSkills();
	
	void ClearGrantedTestSkills();
	
	UAbilitySystemComponent* GetOwnerAbilitySystemComponent() const;
	
	
	static constexpr int32 MaxSlotCount = 2;
	
	static constexpr int32 MaxGrantRetryCount = 50;
	
	
	TArray<FGameplayAbilitySpecHandle> GrantedHandleArray;
	
	FTimerHandle GrantRetryTimerHandle;
	
	int32 GrantRetryCount = 0;
};
