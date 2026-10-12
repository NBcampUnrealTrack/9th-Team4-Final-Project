#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Enum/MotionMatchingTypes.h"
#include "MotionMatchingAnimInstance.generated.h"

class ACharacter;
class UCharacterMovementComponent;
class UMotionMatchingComponent;

// [추가] A-4: 모션매칭 ABP의 부모 클래스.
// 특정 캐릭터 BP로 형변환하지 않고, 소유 캐릭터의 MotionMatchingComponent 값을 매 프레임 복사해 ABP에 제공한다
UCLASS()
class PROJECT_NF_API UMotionMatchingAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// ABP의 SetReferences에서 CharacterMovement 변수를 채울 때 사용
	UFUNCTION(BlueprintPure, Category = "MotionMatching|Animation")
	UCharacterMovementComponent* GetOwningCharacterMovement() const;

	// [추가] A-5: 파쿠르 앞 턱 Transform (MotionMatch의 Traversal 채널이 읽음). BPI_InteractionTransform 대체
	UFUNCTION(BlueprintCallable, Category = "MotionMatching|Animation")
	void SetTraversalInteractionTransform(const FTransform& NewInteractionTransform);

	UFUNCTION(BlueprintPure, Category = "MotionMatching|Animation", meta = (BlueprintThreadSafe))
	FTransform GetTraversalInteractionTransform() const { return TraversalInteractionTransform; }

	// [추가] A-5: PoseSearch 채널 BP에서 사용. 이 클래스의 ABP가 아니면 기본 Transform을 돌려준다
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

	// [추가] A-5
	UPROPERTY(Transient, BlueprintReadOnly, Category = "MotionMatching|Animation")
	FTransform TraversalInteractionTransform = FTransform::Identity;
};
