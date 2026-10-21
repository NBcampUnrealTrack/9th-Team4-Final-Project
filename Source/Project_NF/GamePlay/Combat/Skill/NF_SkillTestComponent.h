#pragma once

#include "CoreMinimal.h"
#include "GAS/Foundation/NF_AbilitySystemComponent.h"
#include "NF_SkillTestComponent.generated.h"

class UNFGA_SkillBase;
class UInputAction;
class UInputMappingContext;
class UEnhancedInputComponent;
class UEnhancedInputLocalPlayerSubsystem;
class APlayerController;
class AController;

USTRUCT()
struct FNF_TestSkillSlotState
{
	GENERATED_BODY()
	UPROPERTY(Transient)
	TWeakObjectPtr<UNFGA_SkillBase> PendingReleaseAbility;
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> Action;
	bool bWaitingForRelease = false;
	bool bPressed = false;
};

/** @Owner: 스킬 담당 (구현: Codex) | 2026-10-12
 * 목적: Pawn에 컴포넌트를 추가하고 GA를 지정하면 E/Q/LeftShift로 실행하는 임시 테스트 ASC.
 * 캐릭터 클래스 수정이나 별도 입력 에셋 없이 사용한다. 기존 ASC가 없는 테스트 Pawn용이다.
 */
UCLASS(ClassGroup = (NF), meta = (BlueprintSpawnableComponent), BlueprintType)
class PROJECT_NF_API UNF_SkillTestComponent : public UNF_AbilitySystemComponent
{
	GENERATED_BODY()

public:
	UNF_SkillTestComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual bool GetShouldTick() const override { return true; }

	// 플레이 전에 GA BP를 드래그/선택한다. 플레이 중 교체는 지원하지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "NF|Skill Test", meta = (DisplayName = "Test Skill Class (E)"))
	TSubclassOf<UNFGA_SkillBase> TestSkillClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "NF|Skill Test", meta = (DisplayName = "Test Skill Class (Q)"))
	TSubclassOf<UNFGA_SkillBase> TestSkillClassQ;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "NF|Skill Test", meta = (DisplayName = "Test Skill Class (Left Shift)"))
	TSubclassOf<UNFGA_SkillBase> TestSkillClassShift;

private:
	void RefreshTestInput();
	void RemoveTestInput();
	void ActivateTestSkill(int32 SlotIndex);
	void BeginTestSkillAim(int32 SlotIndex);
	void ReleaseTestSkillAim(int32 SlotIndex);
	void CancelTestSkillAim(int32 SlotIndex);
	TSubclassOf<UNFGA_SkillBase> GetTestSkillClass(int32 SlotIndex) const;
	FGameplayAbilitySpec* FindTestSkillSpec(int32 SlotIndex);
	static constexpr int32 SlotCount = 3;
	static constexpr int32 FirstTestInputID = 100;
	UPROPERTY(Transient)
	TArray<FNF_TestSkillSlotState> SlotStates;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> TestMapping;

	UPROPERTY(Transient)
	TObjectPtr<UEnhancedInputComponent> TestInput;

	UPROPERTY(Transient)
	TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> InputSubsystem;

	UPROPERTY(Transient)
	TWeakObjectPtr<APlayerController> BoundPlayerController;

	UPROPERTY(Transient)
	TWeakObjectPtr<AController> CachedController;

	TArray<FGameplayAbilitySpecHandle> TestSkillHandles;
};
