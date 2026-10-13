#include "Animation/MotionMatchingAnimInstance.h"
#include "Component/MotionMatchingComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UMotionMatchingAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	CacheOwnerReferences();
	CopyComponentValues();
}


void UMotionMatchingAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// 빙의 변경·리스폰 등으로 소유 캐릭터가 바뀌었을 수 있으므로 비어 있으면 다시 찾는다
	if (IsValid(OwningCharacter) == false || IsValid(MotionMatchingComponent) == false)
	{
		CacheOwnerReferences();
	}

	CopyComponentValues();
}


UCharacterMovementComponent* UMotionMatchingAnimInstance::GetOwningCharacterMovement() const
{
	if (IsValid(OwningCharacter) == false) return nullptr;

	return OwningCharacter->GetCharacterMovement();
}


void UMotionMatchingAnimInstance::SetTraversalInteractionTransform(const FTransform& NewInteractionTransform)
{
	TraversalInteractionTransform = NewInteractionTransform;
}


FTransform UMotionMatchingAnimInstance::GetTraversalInteractionTransformFromAnimInstance(const UAnimInstance* AnimInstance)
{
	const UMotionMatchingAnimInstance* MotionMatchingAnimInstance = Cast<UMotionMatchingAnimInstance>(AnimInstance);
	if (IsValid(MotionMatchingAnimInstance) == false) return FTransform::Identity;

	return MotionMatchingAnimInstance->TraversalInteractionTransform;
}


void UMotionMatchingAnimInstance::CacheOwnerReferences()
{
	OwningCharacter = Cast<ACharacter>(TryGetPawnOwner());
	MotionMatchingComponent = nullptr;
	if (IsValid(OwningCharacter) == false) return;

	MotionMatchingComponent = OwningCharacter->FindComponentByClass<UMotionMatchingComponent>();
}


void UMotionMatchingAnimInstance::CopyComponentValues()
{
	if (IsValid(MotionMatchingComponent) == false) return;

	OwnerGait = MotionMatchingComponent->GetGait();
	OwnerInputState = MotionMatchingComponent->GetCharacterInputState();
	OwnerLandVelocity = MotionMatchingComponent->GetLandVelocity();
	bIsOwnerJustLanded = MotionMatchingComponent->IsJustLanded();
}
