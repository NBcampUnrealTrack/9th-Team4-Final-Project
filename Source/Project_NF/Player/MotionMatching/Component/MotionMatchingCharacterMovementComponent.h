#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Enum/MotionMatchingTypes.h"
#include "MotionMatchingCharacterMovementComponent.generated.h"

class UMotionMatchingComponent;

// 질주·걷기·옆걸음·조준 상태를 이동 정보 꾸러미(SavedMove)에 함께 담아 서버로 보내는 이동 컴포넌트.
// 서버가 클라이언트 이동을 다시 계산할 때 같은 상태로 계산하므로 위치 보정(튕김)이 생기지 않는다.
// 속도·가속 수치 자체는 MotionMatchingComponent가 계산한다 (작업자는 그쪽 Details만 조정).
UCLASS()
class PROJECT_NF_API UMotionMatchingCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

	friend class FSavedMove_MotionMatching;

public:
	UMotionMatchingCharacterMovementComponent(); // [추가] C: GASP 샘플과 같은 이동 기본값

	virtual float GetMaxSpeed() const override;
	virtual float GetMaxAcceleration() const override;
	virtual float GetMaxBrakingDeceleration() const override;
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;
	virtual void ApplyNetworkMovementMode(const uint8 ReceivedMode) override; // [추가]
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

	// 입력 함수(MotionMatchingComponent)가 로컬에서 바로 설정. 서버에서는 꾸러미의 플래그로 설정됨
	void SetMovementInputState(const FMotionMatchingInputState& NewMovementInputState) { MovementInputState = NewMovementInputState; }
	const FMotionMatchingInputState& GetMovementInputState() const { return MovementInputState; }

protected:
	UMotionMatchingComponent* GetMotionMatchingComponent() const;

protected:
	// 이동 계산에 쓰는 입력 상태 (예측·서버 재계산용)
	FMotionMatchingInputState MovementInputState;

	// 이동 계산 중 자주 찾으므로 한 번 찾은 컴포넌트를 기억
	mutable TWeakObjectPtr<UMotionMatchingComponent> CachedMotionMatchingComponent;
};


// 이동 한 번(한 프레임)의 기록. 입력 상태 4가지를 함께 저장해서 서버로 보낸다
class FSavedMove_MotionMatching : public FSavedMove_Character
{
public:
	typedef FSavedMove_Character Super;

	virtual void Clear() override;
	virtual uint8 GetCompressedFlags() const override;
	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
	virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override;
	virtual void PrepMoveFor(ACharacter* C) override;

private:
	bool bIsSavedWantsToSprint = false;
	bool bIsSavedWantsToWalk = false;
	bool bIsSavedWantsToStrafe = false;
	bool bIsSavedWantsToAim = false;
};


// 위 기록 클래스를 이동 컴포넌트가 쓰도록 연결
class FNetworkPredictionData_Client_MotionMatching : public FNetworkPredictionData_Client_Character
{
public:
	typedef FNetworkPredictionData_Client_Character Super;

	FNetworkPredictionData_Client_MotionMatching(const UCharacterMovementComponent& ClientMovement);

	virtual FSavedMovePtr AllocateNewMove() override;
};
