#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"
#include "NF_BuildInputComponent.generated.h"

class UEnhancedInputComponent;
class UInputAction;
class UInputMappingContext;
class APawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNF_OnBuildSlotKeysChanged);

/** Local input only. Attach to the PlayerController, not the replicated PlayerState ASC. */
UCLASS(ClassGroup = (NF), meta = (BlueprintSpawnableComponent))
class PROJECT_NF_API UNF_BuildInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNF_BuildInputComponent();

	UFUNCTION(BlueprintCallable, Category = "NF|Build|Input")
	void InitializeInput(UEnhancedInputComponent* Input);

	/** Existing skill actions may share the new slot keys; movement/interaction actions may not. */
	void SetLegacySkillActions(UInputAction* First, UInputAction* Second);

	/** UI uses 0/1 for slot 1/2. Reject analog keys, modifiers and duplicate slot keys. */
	UFUNCTION(BlueprintCallable, Category = "NF|Build|Input")
	bool SetSlotKey(int32 SlotIndex, FKey Key);

	UFUNCTION(BlueprintPure, Category = "NF|Build|Input")
	FKey GetSlotKey(int32 SlotIndex) const;

	UPROPERTY(BlueprintAssignable, Category = "NF|Build|Input")
	FNF_OnBuildSlotKeysChanged OnSlotKeysChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category = "NF|Build|Input")
	FKey SlotOneKey = EKeys::One;

	UPROPERTY(EditDefaultsOnly, Category = "NF|Build|Input")
	FKey SlotTwoKey = EKeys::Two;

	UPROPERTY(EditDefaultsOnly, Category = "NF|Build|Input")
	int32 MappingPriority = 20;

private:
	UFUNCTION()
	void HandlePawnChanged(APawn* OldPawn, APawn* NewPawn);

	void RebuildMapping();
	void RemoveInputBindings();
	void HandleSlotPressed(int32 SlotIndex);
	void HandleSlotReleased(int32 SlotIndex);

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInputAction>> SlotActions;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInputAction>> LegacySkillActions;

	TWeakObjectPtr<UEnhancedInputComponent> BoundInput;
	TArray<uint32> BindingHandles;
};
