#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Enum/MotionMatchingTypes.h"
#include "MotionMatchingAnimInstance.generated.h"

class ACharacter;
class UCharacterMovementComponent;
class UMotionMatchingComponent;

UCLASS()
class PROJECT_NF_API UMotionMatchingAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "MotionMatching|Animation")
	UCharacterMovementComponent* GetOwningCharacterMovement() const;

	UFUNCTION(BlueprintCallable, Category = "MotionMatching|Animation")
	void SetTraversalInteractionTransform(const FTransform& NewInteractionTransform);

	UFUNCTION(BlueprintPure, Category = "MotionMatching|Animation", meta = (BlueprintThreadSafe))
	FTransform GetTraversalInteractionTransform() const { return TraversalInteractionTransform; }

	UFUNCTION(BlueprintPure, Category = "MotionMatching|Animation", meta = (BlueprintThreadSafe))
	static FTransform GetTraversalInteractionTransformFromAnimInstance(const UAnimInstance* AnimInstance);

protected:
	void CacheOwnerReferences();
	void CopyComponentValues();

protected:
	// 이 ABP를 쓰는 캐릭터 (캐릭터가 아니면 비어 있음)
	UPROPERTY(Transient, BlueprintReadOnly, Category = "MotionMatching|Animation")
	TObjectPtr<ACharacter> OwningCharacter;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "MotionMatching|Animation")
	TObjectPtr<UMotionMatchingComponent> MotionMatchingComponent;

	// 아래는 컴포넌트에서 매 프레임 복사한 값 (애니메이션 스레드에서 안전하게 읽기 위함)
	UPROPERTY(Transient, BlueprintReadOnly, Category = "MotionMatching|Animation")
	EMotionMatchingGait OwnerGait = EMotionMatchingGait::Run;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "MotionMatching|Animation")
	FMotionMatchingInputState OwnerInputState;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "MotionMatching|Animation")
	FVector OwnerLandVelocity = FVector::ZeroVector;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "MotionMatching|Animation")
	bool bIsOwnerJustLanded = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "MotionMatching|Animation")
	FTransform TraversalInteractionTransform = FTransform::Identity;
};
