#include "Component/MotionMatchingCharacterMovementComponent.h"
#include "Component/MotionMatchingComponent.h"
#include "GameFramework/Character.h"

#pragma region MovementComponent
// [추가] C: GASP 샘플 캐릭터(CBP_SandboxCharacter)의 이동 설정을 기본값으로. 설치 대상 캐릭터 BP에 따로 입력할 필요 없음
UMotionMatchingCharacterMovementComponent::UMotionMatchingCharacterMovementComponent()
{
	MaxAcceleration = 800.0f;
	BrakingDecelerationWalking = 1500.0f;
	GroundFriction = 5.0f;
	GravityScale = 1.3f;
	AirControl = 0.25f;
	JumpZVelocity = 500.0f;
	SetCrouchedHalfHeight(60.0f);
	PerchRadiusThreshold = 20.0f;
	bUseSeparateBrakingFriction = true;
	BrakingFrictionFactor = 1.0f;
	RotationRate = FRotator(0.0f, -1.0f, 0.0f);
	bUseControllerDesiredRotation = true;
	MaxWalkSpeed = 500.0f;
	bCanWalkOffLedgesWhenCrouching = true;
	NavAgentProps.bCanCrouch = true;
}


// 걷기·낙하 중 최대 속도는 MotionMatchingComponent가 이 순간의 입력 상태로 계산
float UMotionMatchingCharacterMovementComponent::GetMaxSpeed() const
{
	const UMotionMatchingComponent* MotionMatchingComponent = GetMotionMatchingComponent();
	if (IsValid(MotionMatchingComponent) == false) return Super::GetMaxSpeed();

	// [변경] A-6: 속도 배율(GA·GE용) 적용
	const float SpeedMultiplier = MotionMatchingComponent->GetMovementSpeedMultiplier();
	if (MovementMode == MOVE_Walking || MovementMode == MOVE_NavWalking)
	{
		if (IsCrouching() == true) return MotionMatchingComponent->CalculateMaxCrouchSpeed() * SpeedMultiplier;
		return MotionMatchingComponent->CalculateMaxSpeed() * SpeedMultiplier;
	}

	if (MovementMode == MOVE_Falling) return MotionMatchingComponent->CalculateMaxSpeed() * SpeedMultiplier;
	return Super::GetMaxSpeed();
}


float UMotionMatchingCharacterMovementComponent::GetMaxAcceleration() const
{
	const UMotionMatchingComponent* MotionMatchingComponent = GetMotionMatchingComponent();
	if (IsValid(MotionMatchingComponent) == false) return Super::GetMaxAcceleration();

	return MotionMatchingComponent->CalculateMaxAcceleration();
}


float UMotionMatchingCharacterMovementComponent::GetMaxBrakingDeceleration() const
{
	const UMotionMatchingComponent* MotionMatchingComponent = GetMotionMatchingComponent();
	if (IsValid(MotionMatchingComponent) == false) return Super::GetMaxBrakingDeceleration();
	if (MovementMode != MOVE_Walking && MovementMode != MOVE_NavWalking) return Super::GetMaxBrakingDeceleration();

	return MotionMatchingComponent->CalculateBrakingDeceleration();
}


// 걷는 중 지면 마찰도 이 순간의 입력 상태로 계산
void UMotionMatchingCharacterMovementComponent::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
	const UMotionMatchingComponent* MotionMatchingComponent = GetMotionMatchingComponent();
	if (IsValid(MotionMatchingComponent) == true && IsMovingOnGround() == true)
	{
		Friction = MotionMatchingComponent->CalculateGroundFriction();
	}

	Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
}


// [추가] 서버: 트래버설 전후 '클라이언트 위치를 그대로 믿는' 구간에서는 클라이언트가 보낸 이동 모드를 따르지 않는다.
// 서버가 먼저 끝나 걷기로 바꿨는데, 늦게 도착한 클라이언트의 '비행 중' 보고로 다시 비행에 갇히는 문제 방지
void UMotionMatchingCharacterMovementComponent::ApplyNetworkMovementMode(const uint8 ReceivedMode)
{
	const bool bIsAcceptingRemoteClientPosition = bServerAcceptClientAuthoritativePosition == true
		&& GetOwnerRole() == ROLE_Authority
		&& IsValid(CharacterOwner) == true
		&& CharacterOwner->IsLocallyControlled() == false;
	if (bIsAcceptingRemoteClientPosition == true) return;

	Super::ApplyNetworkMovementMode(ReceivedMode);
}


// 서버: 클라이언트가 보낸 꾸러미의 플래그로 입력 상태 복원
void UMotionMatchingCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	MovementInputState.bIsWantsToSprint = false;
	if ((Flags & FSavedMove_Character::FLAG_Custom_0) != 0)
	{
		MovementInputState.bIsWantsToSprint = true;
	}

	MovementInputState.bIsWantsToWalk = false;
	if ((Flags & FSavedMove_Character::FLAG_Custom_1) != 0)
	{
		MovementInputState.bIsWantsToWalk = true;
	}

	MovementInputState.bIsWantsToStrafe = false;
	if ((Flags & FSavedMove_Character::FLAG_Custom_2) != 0)
	{
		MovementInputState.bIsWantsToStrafe = true;
	}

	MovementInputState.bIsWantsToAim = false;
	if ((Flags & FSavedMove_Character::FLAG_Custom_3) != 0)
	{
		MovementInputState.bIsWantsToAim = true;
	}
}


FNetworkPredictionData_Client* UMotionMatchingCharacterMovementComponent::GetPredictionData_Client() const
{
	if (ClientPredictionData == nullptr)
	{
		// 엔진 패턴: const 함수에서 처음 한 번만 생성
		UMotionMatchingCharacterMovementComponent* MutableThis = const_cast<UMotionMatchingCharacterMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_MotionMatching(*this);
	}

	return ClientPredictionData;
}


UMotionMatchingComponent* UMotionMatchingCharacterMovementComponent::GetMotionMatchingComponent() const
{
	if (CachedMotionMatchingComponent.IsValid() == true) return CachedMotionMatchingComponent.Get();

	const AActor* OwnerActor = GetOwner();
	if (IsValid(OwnerActor) == false) return nullptr;

	CachedMotionMatchingComponent = OwnerActor->FindComponentByClass<UMotionMatchingComponent>();
	return CachedMotionMatchingComponent.Get();
}
#pragma endregion


#pragma region SavedMove
void FSavedMove_MotionMatching::Clear()
{
	Super::Clear();

	bIsSavedWantsToSprint = false;
	bIsSavedWantsToWalk = false;
	bIsSavedWantsToStrafe = false;
	bIsSavedWantsToAim = false;
}


uint8 FSavedMove_MotionMatching::GetCompressedFlags() const
{
	uint8 Flags = Super::GetCompressedFlags();

	if (bIsSavedWantsToSprint == true)
	{
		Flags |= FLAG_Custom_0;
	}

	if (bIsSavedWantsToWalk == true)
	{
		Flags |= FLAG_Custom_1;
	}

	if (bIsSavedWantsToStrafe == true)
	{
		Flags |= FLAG_Custom_2;
	}

	if (bIsSavedWantsToAim == true)
	{
		Flags |= FLAG_Custom_3;
	}

	return Flags;
}


// 입력 상태가 다른 이동끼리는 하나로 합치지 않는다
bool FSavedMove_MotionMatching::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
{
	const FSavedMove_MotionMatching* NewMotionMatchingMove = static_cast<const FSavedMove_MotionMatching*>(NewMove.Get());
	if (NewMotionMatchingMove == nullptr) return false;
	if (bIsSavedWantsToSprint != NewMotionMatchingMove->bIsSavedWantsToSprint) return false;
	if (bIsSavedWantsToWalk != NewMotionMatchingMove->bIsSavedWantsToWalk) return false;
	if (bIsSavedWantsToStrafe != NewMotionMatchingMove->bIsSavedWantsToStrafe) return false;
	if (bIsSavedWantsToAim != NewMotionMatchingMove->bIsSavedWantsToAim) return false;

	return Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
}


// 이동을 기록할 때 현재 입력 상태를 저장
void FSavedMove_MotionMatching::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);

	const UMotionMatchingCharacterMovementComponent* MovementComponent = Cast<UMotionMatchingCharacterMovementComponent>(C->GetCharacterMovement());
	if (IsValid(MovementComponent) == false) return;

	bIsSavedWantsToSprint = MovementComponent->MovementInputState.bIsWantsToSprint;
	bIsSavedWantsToWalk = MovementComponent->MovementInputState.bIsWantsToWalk;
	bIsSavedWantsToStrafe = MovementComponent->MovementInputState.bIsWantsToStrafe;
	bIsSavedWantsToAim = MovementComponent->MovementInputState.bIsWantsToAim;
}


// 서버 보정 후 다시 재생할 때 기록된 입력 상태로 되돌림
void FSavedMove_MotionMatching::PrepMoveFor(ACharacter* C)
{
	Super::PrepMoveFor(C);

	UMotionMatchingCharacterMovementComponent* MovementComponent = Cast<UMotionMatchingCharacterMovementComponent>(C->GetCharacterMovement());
	if (IsValid(MovementComponent) == false) return;

	MovementComponent->MovementInputState.bIsWantsToSprint = bIsSavedWantsToSprint;
	MovementComponent->MovementInputState.bIsWantsToWalk = bIsSavedWantsToWalk;
	MovementComponent->MovementInputState.bIsWantsToStrafe = bIsSavedWantsToStrafe;
	MovementComponent->MovementInputState.bIsWantsToAim = bIsSavedWantsToAim;
}
#pragma endregion


#pragma region PredictionData
FNetworkPredictionData_Client_MotionMatching::FNetworkPredictionData_Client_MotionMatching(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{
}


FSavedMovePtr FNetworkPredictionData_Client_MotionMatching::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_MotionMatching());
}
#pragma endregion
