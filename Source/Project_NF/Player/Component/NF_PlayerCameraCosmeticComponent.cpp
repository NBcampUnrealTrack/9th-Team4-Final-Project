#include "NF_PlayerCameraCosmeticComponent.h"

#include "Component/MotionMatchingComponent.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "Player/Data/NF_PlayerCosmeticDataAsset.h"


UNF_PlayerCameraCosmeticComponent::UNF_PlayerCameraCosmeticComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


void UNF_PlayerCameraCosmeticComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (IsValid(GetOwner()) == false) return;
	
	SpringArmComponent = GetOwner()->FindComponentByClass<USpringArmComponent>();
	MotionMatchingComponent = GetOwner()->FindComponentByClass<UMotionMatchingComponent>();
	
	if (IsValid(SpringArmComponent) == true)
	{
		DefaultCameraArmLength = SpringArmComponent->TargetArmLength;
		CameraCollisionArmLength = DefaultCameraArmLength;
		//BP에 넣어 둔 Target Offset 원래 값 (앉기 높이 보정은 이 위에 더함)
		DefaultTargetOffset = SpringArmComponent->TargetOffset;
	}
	
	//앉을 때 캡슐이 내려가는 높이 계산 + 이동 컴포넌트(앉기 처리)가 끝난 뒤에 이 컴포넌트가 돌도록 순서 지정
	auto* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (IsValid(OwnerCharacter) == true)
	{
		UCharacterMovementComponent* CharacterMovementComponent = OwnerCharacter->GetCharacterMovement();
		if (IsValid(CharacterMovementComponent) == true)
		{
			CrouchCapsuleHeightGap = OwnerCharacter->GetDefaultHalfHeight() - CharacterMovementComponent->GetCrouchedHalfHeight();
			AddTickPrerequisiteComponent(CharacterMovementComponent);
		}
	}
	
	ConfigData();
}


void UNF_PlayerCameraCosmeticComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	auto* OwnerPawn = Cast<APawn>(GetOwner());
	if (IsValid(OwnerPawn) == false) return;
	
	if (OwnerPawn->IsLocallyControlled() == true)
	{
		//꺼진 기능은 계산을 건너뜀 (보정 0, Target Offset은 BP 값 그대로)
		if (bIsSprintCameraEnabled == true)
		{
			UpdateSprintCameraArmOffset(DeltaTime);
		}
		if (bIsCrouchCameraEnabled == true)
		{
			UpdateCrouchCameraArmOffset(DeltaTime);
		}
		UpdateCameraArmLength(DeltaTime);
	}
}


void UNF_PlayerCameraCosmeticComponent::ConfigData()
{
	checkf(IsValid(NF_PlayerCosmeticDataAsset) == true, TEXT("[%s]의 [NF_PlayerCameraCosmeticComponent]의 [NF_PlayerCosmeticDataAsset]가 삽입되어있지않습니다."), *GetNameSafe(GetOwner()));
	bIsSprintCameraEnabled = NF_PlayerCosmeticDataAsset->bIsSprintCameraEnabled;
	bIsCrouchCameraEnabled = NF_PlayerCosmeticDataAsset->bIsCrouchCameraEnabled;
	
	//꺼진 기능은 커브가 비어 있어도 멈추지 않음
	if (bIsSprintCameraEnabled == true)
	{
		checkf(IsValid(NF_PlayerCosmeticDataAsset->SprintCameraFarCurve) == true, TEXT("[%s]의 [NF_PlayerCameraCosmeticComponent]의 [NF_PlayerCosmeticDataAsset]의 [SprintCameraFarCurve]가 삽입되어있지않습니다."), *GetNameSafe(GetOwner()));
	}
	if (bIsCrouchCameraEnabled == true)
	{
		checkf(IsValid(NF_PlayerCosmeticDataAsset->CrouchCameraCloserCurve) == true, TEXT("[%s]의 [NF_PlayerCameraCosmeticComponent]의 [NF_PlayerCosmeticDataAsset]의 [CrouchCameraCloserCurve]가 삽입되어있지않습니다."), *GetNameSafe(GetOwner()));
	}
	
	SprintCameraArmCurve = NF_PlayerCosmeticDataAsset->SprintCameraFarCurve;
	CrouchCameraArmCurve = NF_PlayerCosmeticDataAsset->CrouchCameraCloserCurve;
	CameraCollisionNearSpeed = NF_PlayerCosmeticDataAsset->CameraCollisionNearSpeed;
	CameraCollisionFarSpeed = NF_PlayerCosmeticDataAsset->CameraCollisionFarSpeed;
}


void UNF_PlayerCameraCosmeticComponent::UpdateSprintCameraArmOffset(float DeltaTime)
{
	if (IsValid(MotionMatchingComponent) == false) return;
	if (IsValid(SprintCameraArmCurve) == false) return;
	
	//달리기 최고 속도 = 0, 질주 최고 속도 = 1
	const float RunMaxSpeed = MotionMatchingComponent->GetRunSpeeds().X;
	const float SprintMaxSpeed = MotionMatchingComponent->GetSprintSpeeds().X;
	if (SprintMaxSpeed <= RunMaxSpeed) return;
	
	//현재 속도가 두 지점 사이 어디쯤인지 (0 ~ 1)
	float SprintSpeedProgress = (GetOwner()->GetVelocity().Size2D() - RunMaxSpeed) / (SprintMaxSpeed - RunMaxSpeed);
	if (SprintSpeedProgress < 0.0f)
	{
		SprintSpeedProgress = 0.0f;
	}
	if (SprintSpeedProgress > 1.0f)
	{
		SprintSpeedProgress = 1.0f;
	}
	
	//커브에 그려진 X 범위(시작 ~ 끝)를 읽음 (0~1, 0~100 등 자유)
	float SprintCameraCurveMinX = 0.0f;
	float SprintCameraCurveMaxX = 0.0f;
	SprintCameraArmCurve->GetTimeRange(SprintCameraCurveMinX, SprintCameraCurveMaxX);
	
	//진행도(0~1)를 커브의 X 범위로 늘려서 넣음. Y = 기본 거리에 더할 길이(cm)
	const float SprintCameraCurveX = SprintCameraCurveMinX + (SprintSpeedProgress * (SprintCameraCurveMaxX - SprintCameraCurveMinX));
	SprintCameraArmOffset = SprintCameraArmCurve->GetFloatValue(SprintCameraCurveX);
}


void UNF_PlayerCameraCosmeticComponent::UpdateCrouchCameraArmOffset(float DeltaTime)
{
	if (IsValid(CrouchCameraArmCurve) == false) return;
	
	auto* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (IsValid(OwnerCharacter) == false) return;
	
	const UCharacterMovementComponent* CharacterMovementComponent = OwnerCharacter->GetCharacterMovement();
	if (IsValid(CharacterMovementComponent) == false) return;
	
	//커브 X = 시간(초). 마지막 키의 X = 다 가까워지는 데 걸리는 시간
	float CrouchCameraCurveMinTime = 0.0f;
	float CrouchCameraCurveMaxTime = 0.0f;
	CrouchCameraArmCurve->GetTimeRange(CrouchCameraCurveMinTime, CrouchCameraCurveMaxTime);
	
	//입력 방식(토글/홀드)과 무관하게 앉아 있는 상태면 시간 증가, 아니면 감소 (같은 커브를 거꾸로 타고 돌아옴)
	if (CharacterMovementComponent->IsCrouching() == true)
	{
		CrouchCameraElapsedTime += DeltaTime;
	}
	else
	{
		CrouchCameraElapsedTime -= DeltaTime;
	}
	
	if (CrouchCameraElapsedTime > CrouchCameraCurveMaxTime)
	{
		CrouchCameraElapsedTime = CrouchCameraCurveMaxTime;
	}
	if (CrouchCameraElapsedTime < CrouchCameraCurveMinTime)
	{
		CrouchCameraElapsedTime = CrouchCameraCurveMinTime;
	}
	
	//커브 Y = 가까워질 길이(cm). 최종 거리에서 뺀다
	CrouchCameraArmOffset = CrouchCameraArmCurve->GetFloatValue(CrouchCameraElapsedTime);
	
	//진행 비율 (0 ~ 1) = 현재 커브 값 / 커브 최대값
	float CrouchProgressRatio = 0.0f;
	const float CrouchCameraArmMaxOffset = CrouchCameraArmCurve->GetFloatValue(CrouchCameraCurveMaxTime);
	if (CrouchCameraArmMaxOffset > 0.0f)
	{
		CrouchProgressRatio = CrouchCameraArmOffset / CrouchCameraArmMaxOffset;
	}
	
	//캡슐이 즉시 내려간 만큼 다시 올려 두고, 진행 비율만큼 내려 줌 (일어설 때는 반대) → 높이도 커브 타이밍을 따름
	float CrouchCameraHeightOffset = 0.0f;
	if (CharacterMovementComponent->IsCrouching() == true)
	{
		CrouchCameraHeightOffset = CrouchCapsuleHeightGap;
	}
	CrouchCameraHeightOffset -= CrouchCapsuleHeightGap * CrouchProgressRatio;
	
	if (IsValid(SpringArmComponent) == false) return;
	SpringArmComponent->TargetOffset = DefaultTargetOffset + FVector(0.0f, 0.0f, CrouchCameraHeightOffset);
}


void UNF_PlayerCameraCosmeticComponent::UpdateCameraArmLength(float DeltaTime)
{
	auto* OwnerPawn = Cast<APawn>(GetOwner());
	if (IsValid(OwnerPawn) == false) return;
	if (IsValid(SpringArmComponent) == false) return;
	
	//질주로 벌어질 수 있는 최대 길이(커브의 마지막 X 값)까지 벽 검사
	float SprintCameraArmMaxOffset = 0.0f;
	if (IsValid(SprintCameraArmCurve) == true)
	{
		float SprintCameraCurveMinX = 0.0f;
		float SprintCameraCurveMaxX = 0.0f;
		SprintCameraArmCurve->GetTimeRange(SprintCameraCurveMinX, SprintCameraCurveMaxX);
		SprintCameraArmMaxOffset = SprintCameraArmCurve->GetFloatValue(SprintCameraCurveMaxX);
	}
	const float MaxCameraArmLength = DefaultCameraArmLength + SprintCameraArmMaxOffset;
	
	//TargetOffset(높이, 월드 기준)은 그대로, SocketOffset(어깨 위치)은 컨트롤 회전 기준으로 돌려서 검사 시작점에 더함
	const FVector ArmOrigin = SpringArmComponent->GetComponentLocation() + SpringArmComponent->TargetOffset + OwnerPawn->GetControlRotation().RotateVector(SpringArmComponent->SocketOffset);
	const FVector DesiredCameraLocation = ArmOrigin - (OwnerPawn->GetControlRotation().Vector() * MaxCameraArmLength);
	
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NF_PlayerCameraArm), false, OwnerPawn);
	
	FHitResult HitResult;
	bool bIsCameraArmBlocked = GetWorld()->SweepSingleByObjectType(HitResult, ArmOrigin, DesiredCameraLocation,
		FQuat::Identity, ObjectQueryParams, FCollisionShape::MakeSphere(12.0f), QueryParams);
	
	//벽 제한 거리: 벽이 없으면 최대 길이까지 허용, 있으면 벽까지
	float TargetCollisionArmLength = MaxCameraArmLength;
	if (bIsCameraArmBlocked == true)
	{
		TargetCollisionArmLength = HitResult.Distance;
	}
	
	//벽 제한 거리만 Near/Far 빠르기로 보간
	float CameraCollisionInterpSpeed = CameraCollisionFarSpeed;
	if (TargetCollisionArmLength < CameraCollisionArmLength)
	{
		CameraCollisionInterpSpeed = CameraCollisionNearSpeed;
	}
	CameraCollisionArmLength = FMath::FInterpTo(CameraCollisionArmLength, TargetCollisionArmLength, DeltaTime, CameraCollisionInterpSpeed);
	
	//질주(+)·앉기(-) 거리는 커브 그대로, 벽 제한 거리보다 길면 자름
	float FinalCameraArmLength = DefaultCameraArmLength + SprintCameraArmOffset - CrouchCameraArmOffset;
	if (FinalCameraArmLength > CameraCollisionArmLength)
	{
		FinalCameraArmLength = CameraCollisionArmLength;
	}
	
	SpringArmComponent->TargetArmLength = FinalCameraArmLength;
}
