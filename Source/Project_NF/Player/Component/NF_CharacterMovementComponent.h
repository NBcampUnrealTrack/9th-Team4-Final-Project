#pragma once

#include "CoreMinimal.h"
#include "Component/MotionMatchingCharacterMovementComponent.h"
#include "NF_CharacterMovementComponent.generated.h"

// 플러그인 이동 컴포넌트를 그대로 물려받고, 다른 캐릭터 위에 얹혔을 때의 JumpOff만 조정한다
UCLASS()
class PROJECT_NF_API UNF_CharacterMovementComponent : public UMotionMatchingCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UNF_CharacterMovementComponent();

	virtual void JumpOff(AActor* MovementBaseActor) override;
	virtual FVector GetBestDirectionOffActor(AActor* BaseActor) const override;

	// 공격(Player.State.Attack) 중에는 이동 방향 대신 카메라(컨트롤러) Yaw를 회전 목표로 사용
	virtual FRotator ComputeOrientToMovementRotation(const FRotator& CurrentRotation, float DeltaTime, FRotator& DeltaRotation) const override;

protected:
	// 옆으로 밀리는 최대 속도 = 최대 속도 × 이 값 (엔진 기본 0.85)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF_Movement|JumpOff", meta = (ClampMin = "0.0"))
	float JumpOffHorizontalSpeedScale = 0.3f;
};