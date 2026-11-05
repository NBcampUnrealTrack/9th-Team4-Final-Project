#include "NF_CharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GAS/Tags/NFTag_Player.h"


UNF_CharacterMovementComponent::UNF_CharacterMovementComponent()
{
	// 위로 띄우는 세기 = JumpZVelocity × 이 값 (엔진 기본 0.5)
	JumpOffJumpZFactor = 0.2f;
}


// 엔진 JumpOff와 같은 흐름(재진입 방지, 커스텀 중력 처리 포함). 옆으로 밀리는 배율만 JumpOffHorizontalSpeedScale로 바꿈
void UNF_CharacterMovementComponent::JumpOff(AActor* MovementBaseActor)
{
	if (bPerformingJumpOff == true) return;
	if (IsValid(CharacterOwner) == false) return;

	bPerformingJumpOff = true;

	const float HorizontalMaxSpeed = GetMaxSpeed() * JumpOffHorizontalSpeedScale;
	Velocity += GetBestDirectionOffActor(MovementBaseActor) * HorizontalMaxSpeed;

	if (ProjectToGravityFloor(Velocity).Size() > HorizontalMaxSpeed)
	{
		Velocity = Velocity.GetSafeNormal() * HorizontalMaxSpeed;
	}

	if (HasCustomGravity() == true)
	{
		SetGravitySpaceZ(Velocity, JumpOffJumpZFactor * JumpZVelocity);
	}
	else
	{
		Velocity.Z = JumpOffJumpZFactor * JumpZVelocity;
	}

	SetMovementMode(MOVE_Falling);
	bPerformingJumpOff = false;
}


// 엔진 기본은 무작위 방향. 밟고 있는 액터 중심에서 내 위치로 향하는 수평 방향(바깥쪽)으로 바꿈
FVector UNF_CharacterMovementComponent::GetBestDirectionOffActor(AActor* BaseActor) const
{
	if (IsValid(BaseActor) == false) return Super::GetBestDirectionOffActor(BaseActor);
	if (IsValid(CharacterOwner) == false) return Super::GetBestDirectionOffActor(BaseActor);

	const FVector AwayFromBase = ProjectToGravityFloor(CharacterOwner->GetActorLocation() - BaseActor->GetActorLocation());

	// 정확히 머리 꼭대기 중앙이면 방향을 정할 수 없으므로 엔진 기본(무작위)으로
	if (AwayFromBase.IsNearlyZero() == true) return Super::GetBestDirectionOffActor(BaseActor);

	return AwayFromBase.GetSafeNormal();
}


// Player.State.Attack 태그가 있는 동안 캐릭터가 카메라 Yaw를 바라보게 한다
// 클라(예측)와 서버의 이동 계산 양쪽에서 실행되고, 컨트롤러가 없는 SimulatedProxy에서는 엔진 기본 동작
FRotator UNF_CharacterMovementComponent::ComputeOrientToMovementRotation(const FRotator& CurrentRotation, float DeltaTime, FRotator& DeltaRotation) const
{
	if (IsValid(CharacterOwner) == false) return Super::ComputeOrientToMovementRotation(CurrentRotation, DeltaTime, DeltaRotation);
	if (IsValid(CharacterOwner->GetController()) == false) return Super::ComputeOrientToMovementRotation(CurrentRotation, DeltaTime, DeltaRotation);

	const UAbilitySystemComponent* AbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(CharacterOwner);
	if (IsValid(AbilitySystemComponent) == false) return Super::ComputeOrientToMovementRotation(CurrentRotation, DeltaTime, DeltaRotation);
	if (AbilitySystemComponent->HasMatchingGameplayTag(NFTag_Player::Player_State_Attack) == false) return Super::ComputeOrientToMovementRotation(CurrentRotation, DeltaTime, DeltaRotation);

	return FRotator(0.0f, CharacterOwner->GetControlRotation().Yaw, 0.0f);
}
