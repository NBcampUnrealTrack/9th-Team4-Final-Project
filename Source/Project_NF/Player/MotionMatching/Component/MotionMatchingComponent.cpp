#include "Component/MotionMatchingComponent.h"
#include "Component/MotionMatchingCharacterMovementComponent.h" // [추가] A-2
#include "Animation/MotionMatchingAnimInstance.h" // [추가] A-5
#include "Actor/MotionMatchingTraversableBlock.h"
#include "Curves/CurveFloat.h" // [추가]
#include "KismetAnimationLibrary.h" // [추가]
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetStringLibrary.h"
#include "ChooserFunctionLibrary.h"
#include "IObjectChooser.h"
#include "PoseSearch/PoseSearchLibrary.h"
#include "PoseSearch/PoseSearchResult.h"
#include "MotionWarpingComponent.h"
#include "AnimationWarpingLibrary.h"
#include "Kismet/GameplayStatics.h" // [추가]
#include "Sound/SoundBase.h" // [추가]
#include "HAL/IConsoleManager.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogMotionMatching, Log, All);

namespace MotionMatchingTraversal
{
	// 디버그 콘솔 변수 (DefaultEngine.ini에 등록되어 있음)
	const TCHAR* DrawDebugLevelCvarName = TEXT("DDCvar.Traversal.DrawDebugLevel");
	const TCHAR* DrawDebugDurationCvarName = TEXT("DDCvar.Traversal.DrawDebugDuration");

	const FName FrontLedgeWarpTargetName = TEXT("FrontLedge");
	const FName BackLedgeWarpTargetName = TEXT("BackLedge");
	const FName BackFloorWarpTargetName = TEXT("BackFloor");
	const FName DistanceFromLedgeCurveName = TEXT("Distance_From_Ledge");

	int32 GetDrawDebugLevel()
	{
		IConsoleVariable* ConsoleVariable = IConsoleManager::Get().FindConsoleVariable(DrawDebugLevelCvarName);
		if (ConsoleVariable == nullptr) return 0;
		return ConsoleVariable->GetInt();
	}


	float GetDrawDebugDuration()
	{
		IConsoleVariable* ConsoleVariable = IConsoleManager::Get().FindConsoleVariable(DrawDebugDurationCvarName);
		if (ConsoleVariable == nullptr) return 1.0f;
		return ConsoleVariable->GetFloat();
	}
}


UMotionMatchingComponent::UMotionMatchingComponent()
{
	// [변경] 캐릭터 이동 계산(CharacterMovement) 직전에 매 틱 실행 (기존 PlayerPreMovementTickComponent 역할)
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	SetIsReplicatedByDefault(true);
}


void UMotionMatchingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ThisClass, TraversalResult, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ThisClass, CharacterInputState, COND_SkipOwner); // [추가]
	DOREPLIFETIME(ThisClass, MovementSpeedMultiplier); // [추가] A-6: 서버가 정한 값이 조종 클라이언트에도 도착해야 하므로 조건 없음

	// [추가] v0.0.6: 기능 켜기/끄기 (조종 클라이언트 포함 모두에게)
	DOREPLIFETIME(ThisClass, bIsSprintEnabled);
	DOREPLIFETIME(ThisClass, bIsWalkToggleEnabled);
	DOREPLIFETIME(ThisClass, bIsStrafeEnabled);
	DOREPLIFETIME(ThisClass, bIsAimEnabled);
	DOREPLIFETIME(ThisClass, bIsCrouchEnabled);
	DOREPLIFETIME(ThisClass, bIsJumpEnabled);
	DOREPLIFETIME(ThisClass, bIsTraversalEnabled);
}


// [추가] CharacterMovement가 이 컴포넌트 다음에 틱하도록 등록 + 착지·점프 알림 연결
void UMotionMatchingComponent::BeginPlay()
{
	Super::BeginPlay();

	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (IsValid(OwnerCharacter) == false) return;

	UCharacterMovementComponent* CharacterMovementComponent = OwnerCharacter->GetCharacterMovement();
	if (IsValid(CharacterMovementComponent) == false) return;

	CharacterMovementComponent->AddTickPrerequisiteComponent(this);
	OwnerCharacter->LandedDelegate.AddDynamic(this, &ThisClass::HandleOwnerLanded);
	OwnerCharacter->MovementModeChangedDelegate.AddDynamic(this, &ThisClass::HandleOwnerMovementModeChanged);

	// 다른 클라이언트 화면의 캐릭터는 착지 알림이 오지 않으므로 지면 상태 변화를 직접 감지
	if (OwnerCharacter->GetLocalRole() == ROLE_SimulatedProxy)
	{
		OwnerCharacter->OnCharacterMovementUpdated.AddDynamic(this, &ThisClass::HandleOwnerMovementUpdated);
	}
}


// [추가] 매 틱: 회전/속도 값을 갱신한 뒤 CharacterMovement가 이동 계산
void UMotionMatchingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateRotation_PreCMC();
	UpdateMovement_PreCMC();
}


#pragma region Feature
// [추가] v0.0.6
void UMotionMatchingComponent::SetFeatureEnabled(EMotionMatchingFeature Feature, bool bIsEnabled)
{
	const AActor* OwnerActor = GetOwner();
	if (IsValid(OwnerActor) == false) return;

	ApplyFeatureEnabled(Feature, bIsEnabled);
	if (OwnerActor->HasAuthority() == true) return;

	ServerRPC_SetFeatureEnabled(Feature, bIsEnabled);
}


// [추가] v0.0.6
bool UMotionMatchingComponent::IsFeatureEnabled(EMotionMatchingFeature Feature) const
{
	switch (Feature)
	{
	case EMotionMatchingFeature::Sprint: return bIsSprintEnabled;
	case EMotionMatchingFeature::WalkToggle: return bIsWalkToggleEnabled;
	case EMotionMatchingFeature::Strafe: return bIsStrafeEnabled;
	case EMotionMatchingFeature::Aim: return bIsAimEnabled;
	case EMotionMatchingFeature::Crouch: return bIsCrouchEnabled;
	case EMotionMatchingFeature::Jump: return bIsJumpEnabled;
	case EMotionMatchingFeature::Traversal: return bIsTraversalEnabled;
	default: return false;
	}
}


// [추가] v0.0.6: 값을 바꾸고, 끈 기능의 현재 상태(질주 중, 앉은 상태 등)를 푼다
void UMotionMatchingComponent::ApplyFeatureEnabled(EMotionMatchingFeature Feature, bool bIsEnabled)
{
	switch (Feature)
	{
	case EMotionMatchingFeature::Sprint: bIsSprintEnabled = bIsEnabled; break;
	case EMotionMatchingFeature::WalkToggle: bIsWalkToggleEnabled = bIsEnabled; break;
	case EMotionMatchingFeature::Strafe: bIsStrafeEnabled = bIsEnabled; break;
	case EMotionMatchingFeature::Aim: bIsAimEnabled = bIsEnabled; break;
	case EMotionMatchingFeature::Crouch: bIsCrouchEnabled = bIsEnabled; break;
	case EMotionMatchingFeature::Jump: bIsJumpEnabled = bIsEnabled; break;
	case EMotionMatchingFeature::Traversal: bIsTraversalEnabled = bIsEnabled; break;
	default: break;
	}

	ClearDisabledFeatureStates();
}


// [추가] v0.0.6: 꺼진 기능의 입력 상태를 지우고, 앉기가 꺼졌으면 일어선다
void UMotionMatchingComponent::ClearDisabledFeatureStates()
{
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (IsValid(OwnerCharacter) == false) return;

	if (bIsCrouchEnabled == false && OwnerCharacter->bIsCrouched == true)
	{
		if (OwnerCharacter->IsLocallyControlled() == true || OwnerCharacter->HasAuthority() == true)
		{
			OwnerCharacter->UnCrouch();
		}
	}

	// 입력 상태는 조종하는 쪽에서만 바꾼다 (서버·다른 화면에는 기존 경로로 전달됨)
	if (OwnerCharacter->IsLocallyControlled() == false) return;

	const FMotionMatchingInputState FilteredInputState = FilterDisabledFeatures(CharacterInputState);
	const bool bIsChanged = FilteredInputState.bIsWantsToSprint != CharacterInputState.bIsWantsToSprint
		|| FilteredInputState.bIsWantsToWalk != CharacterInputState.bIsWantsToWalk
		|| FilteredInputState.bIsWantsToStrafe != CharacterInputState.bIsWantsToStrafe
		|| FilteredInputState.bIsWantsToAim != CharacterInputState.bIsWantsToAim;
	if (bIsChanged == false) return;

	UpdateCharacterInputState(FilteredInputState);
}


// [추가] v0.0.6: 꺼진 기능은 입력 상태가 남아 있어도 없는 것으로 본다
FMotionMatchingInputState UMotionMatchingComponent::FilterDisabledFeatures(const FMotionMatchingInputState& InputState) const
{
	FMotionMatchingInputState FilteredInputState = InputState;
	if (bIsSprintEnabled == false) FilteredInputState.bIsWantsToSprint = false;
	if (bIsWalkToggleEnabled == false) FilteredInputState.bIsWantsToWalk = false;
	if (bIsStrafeEnabled == false) FilteredInputState.bIsWantsToStrafe = false;
	if (bIsAimEnabled == false) FilteredInputState.bIsWantsToAim = false;
	return FilteredInputState;
}


// [추가] v0.0.6
void UMotionMatchingComponent::ServerRPC_SetFeatureEnabled_Implementation(EMotionMatchingFeature Feature, bool bIsEnabled)
{
	ApplyFeatureEnabled(Feature, bIsEnabled);
}


// [추가] v0.0.6: 서버에서 바뀐 값이 도착하면 이 화면의 상태도 정리
void UMotionMatchingComponent::OnRep_FeatureFlags()
{
	ClearDisabledFeatureStates();
}
#pragma endregion


#pragma region Movement
void UMotionMatchingComponent::UpdateRotation_PreCMC()
{
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (IsValid(OwnerCharacter) == false) return;

	UCharacterMovementComponent* CharacterMovementComponent = OwnerCharacter->GetCharacterMovement();
	if (IsValid(CharacterMovementComponent) == false) return;

	// Strafe/Aim 중이면 카메라 방향을 보고, 아니면 이동 방향을 본다
	const FMotionMatchingInputState MovementInputState = GetMovementInputState(); // [변경] A-2: 예측되는 입력 상태 사용
	if (MovementInputState.bIsWantsToStrafe == true || MovementInputState.bIsWantsToAim == true)
	{
		CharacterMovementComponent->bUseControllerDesiredRotation = true;
		CharacterMovementComponent->bOrientRotationToMovement = false;
	}
	else
	{
		CharacterMovementComponent->bUseControllerDesiredRotation = false;
		CharacterMovementComponent->bOrientRotationToMovement = true;
	}

	// 공중에서는 초당 200도, 땅에서는 -1(즉시 회전)
	if (CharacterMovementComponent->IsFalling() == true)
	{
		CharacterMovementComponent->RotationRate = FRotator(0.0f, 200.0f, 0.0f);
	}
	else
	{
		CharacterMovementComponent->RotationRate = FRotator(0.0f, -1.0f, 0.0f);
	}
}


void UMotionMatchingComponent::UpdateMovement_PreCMC()
{
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (IsValid(OwnerCharacter) == false) return;

	UCharacterMovementComponent* CharacterMovementComponent = OwnerCharacter->GetCharacterMovement();
	if (IsValid(CharacterMovementComponent) == false) return;

	// [변경] A-6: 바뀌었을 때 알림
	const EMotionMatchingGait PreviousGait = Gait;
	Gait = GetDesiredGait();
	if (Gait != PreviousGait)
	{
		OnGaitChanged.Broadcast(Gait, PreviousGait);
	}

	CharacterMovementComponent->MaxAcceleration = CalculateMaxAcceleration();
	CharacterMovementComponent->BrakingDecelerationWalking = CalculateBrakingDeceleration();
	CharacterMovementComponent->GroundFriction = CalculateGroundFriction();

	CharacterMovementComponent->MaxWalkSpeed = CalculateMaxSpeed();
	CharacterMovementComponent->MaxWalkSpeedCrouched = CalculateMaxCrouchSpeed();
}


// [변경] PC 전용: 아날로그 스틱 판정 제거 → 질주 가능하면 Sprint, 걷기 토글이면 Walk, 아니면 Run
EMotionMatchingGait UMotionMatchingComponent::GetDesiredGait() const
{
	if (CanSprint() == true) return EMotionMatchingGait::Sprint;
	if (GetMovementInputState().bIsWantsToWalk == true) return EMotionMatchingGait::Walk; // [변경] A-2
	return EMotionMatchingGait::Run;
}


bool UMotionMatchingComponent::CanSprint() const
{
	if (GetMovementInputState().bIsWantsToSprint == false) return false; // [변경] A-2

	const ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (IsValid(OwnerCharacter) == false) return false;

	const UCharacterMovementComponent* CharacterMovementComponent = OwnerCharacter->GetCharacterMovement();
	if (IsValid(CharacterMovementComponent) == false) return false;
	if (CharacterMovementComponent->bOrientRotationToMovement == true) return true;

	// Strafe 중에는 캐릭터가 보는 방향과 이동 방향 차이가 50도 미만일 때만 질주 가능
	FVector MovementDirection = CharacterMovementComponent->GetCurrentAcceleration();
	if (OwnerCharacter->IsLocallyControlled() == true)
	{
		MovementDirection = OwnerCharacter->GetPendingMovementInputVector();
	}

	const FRotator DeltaRotation = UKismetMathLibrary::NormalizedDeltaRotator(OwnerCharacter->GetActorRotation(), MovementDirection.Rotation());
	if (FMath::Abs(DeltaRotation.Yaw) < 50.0f) return true;
	return false;
}


float UMotionMatchingComponent::CalculateMaxAcceleration() const
{
	if (GetDesiredGait() != EMotionMatchingGait::Sprint) return DefaultMaxAcceleration; // [변경] A-6: Details 값 사용

	const AActor* OwnerActor = GetOwner();
	if (IsValid(OwnerActor) == false) return DefaultMaxAcceleration;

	// 질주 중에는 속도가 오를수록 가속도를 줄인다 (기본: 300 → 700일 때 800 → 300)
	const float GroundSpeed = OwnerActor->GetVelocity().Size2D();
	return UKismetMathLibrary::MapRangeClamped(GroundSpeed, SprintAccelerationSpeedRange.X, SprintAccelerationSpeedRange.Y, SprintAccelerationRange.X, SprintAccelerationRange.Y);
}


float UMotionMatchingComponent::CalculateBrakingDeceleration() const
{
	if (HasMovementInputVector() == true) return BrakingDecelerationWithInput; // [변경] A-6
	return BrakingDecelerationWithoutInput;
}


float UMotionMatchingComponent::CalculateBrakingFriction() const
{
	const ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (IsValid(OwnerCharacter) == false) return 0.0f;

	const UCharacterMovementComponent* CharacterMovementComponent = OwnerCharacter->GetCharacterMovement();
	if (IsValid(CharacterMovementComponent) == false) return 0.0f;
	if (CharacterMovementComponent->GetCurrentAcceleration().IsNearlyZero() == true) return BrakingFrictionWithoutInput; // [변경] A-6
	return 0.0f;
}


// [추가] A-6
void UMotionMatchingComponent::SetMovementSpeedMultiplier(float NewMovementSpeedMultiplier)
{
	MovementSpeedMultiplier = FMath::Max(0.0f, NewMovementSpeedMultiplier);
}


float UMotionMatchingComponent::CalculateGroundFriction() const
{
	if (GetDesiredGait() != EMotionMatchingGait::Sprint) return DefaultGroundFriction; // [변경] A-6: Details 값 사용

	const AActor* OwnerActor = GetOwner();
	if (IsValid(OwnerActor) == false) return DefaultGroundFriction;

	// 질주 중에는 속도가 오를수록 마찰을 줄인다 (기본: 0 → 500일 때 5 → 3)
	const float GroundSpeed = OwnerActor->GetVelocity().Size2D();
	return UKismetMathLibrary::MapRangeClamped(GroundSpeed, SprintFrictionSpeedRange.X, SprintFrictionSpeedRange.Y, SprintGroundFrictionRange.X, SprintGroundFrictionRange.Y);
}


float UMotionMatchingComponent::CalculateMaxSpeed() const
{
	const ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (IsValid(OwnerCharacter) == false) return 0.0f;

	const UCharacterMovementComponent* CharacterMovementComponent = OwnerCharacter->GetCharacterMovement();
	if (IsValid(CharacterMovementComponent) == false) return 0.0f;

	const float StrafeSpeedMap = CalculateStrafeSpeedMap(CharacterMovementComponent->bUseControllerDesiredRotation);
	const FVector GaitSpeeds = GetGaitSpeeds();

	if (StrafeSpeedMap < 1.0f) return UKismetMathLibrary::MapRangeClamped(StrafeSpeedMap, 0.0f, 1.0f, GaitSpeeds.X, GaitSpeeds.Y);
	return UKismetMathLibrary::MapRangeClamped(StrafeSpeedMap, 1.0f, 2.0f, GaitSpeeds.Y, GaitSpeeds.Z);
}


float UMotionMatchingComponent::CalculateMaxCrouchSpeed() const
{
	const ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (IsValid(OwnerCharacter) == false) return 0.0f;

	const UCharacterMovementComponent* CharacterMovementComponent = OwnerCharacter->GetCharacterMovement();
	if (IsValid(CharacterMovementComponent) == false) return 0.0f;

	bool bIsStrafing = false;
	if (CharacterMovementComponent->bOrientRotationToMovement == false)
	{
		bIsStrafing = true;
	}

	const float StrafeSpeedMap = CalculateStrafeSpeedMap(bIsStrafing);
	if (StrafeSpeedMap < 1.0f) return UKismetMathLibrary::MapRangeClamped(StrafeSpeedMap, 0.0f, 1.0f, CrouchSpeeds.X, CrouchSpeeds.Y);
	return UKismetMathLibrary::MapRangeClamped(StrafeSpeedMap, 1.0f, 2.0f, CrouchSpeeds.Y, CrouchSpeeds.Z);
}


// [추가] 0 = 앞, 1 = 옆, 2 = 뒤 (Strafe 중일 때만 커브에서 읽음, 아니면 0)
float UMotionMatchingComponent::CalculateStrafeSpeedMap(bool bIsStrafing) const
{
	if (bIsStrafing == false) return 0.0f;
	if (IsValid(StrafeSpeedMapCurve) == false) return 0.0f;

	const AActor* OwnerActor = GetOwner();
	if (IsValid(OwnerActor) == false) return 0.0f;

	const float Direction = UKismetAnimationLibrary::CalculateDirection(OwnerActor->GetVelocity(), OwnerActor->GetActorRotation());
	return StrafeSpeedMapCurve->GetFloatValue(FMath::Abs(Direction));
}


// [추가] 현재 Gait의 속도 묶음 (X = 앞, Y = 옆, Z = 뒤)
FVector UMotionMatchingComponent::GetGaitSpeeds() const
{
	const EMotionMatchingGait DesiredGait = GetDesiredGait(); // [변경] A-2
	if (DesiredGait == EMotionMatchingGait::Run) return RunSpeeds;
	if (DesiredGait == EMotionMatchingGait::Sprint) return SprintSpeeds;
	return WalkSpeeds;
}


bool UMotionMatchingComponent::HasMovementInputVector() const
{
	const ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (IsValid(OwnerCharacter) == false) return false;
	// [변경] A-2: 서버에서도 같은 값이 나오도록 이동 컴포넌트의 현재 가속(입력) 사용
	const UCharacterMovementComponent* CharacterMovementComponent = OwnerCharacter->GetCharacterMovement();
	if (IsValid(CharacterMovementComponent) == false) return false;
	if (CharacterMovementComponent->GetCurrentAcceleration().IsNearlyZero() == true) return false;
	return true;
}


// [추가] A-2: 본인·서버는 이동 컴포넌트의 예측 입력 상태, 다른 클라이언트 화면은 복제된 입력 상태
FMotionMatchingInputState UMotionMatchingComponent::GetMovementInputState() const
{
	// [변경] v0.0.6: 꺼진 기능은 걸러서 반환
	const AActor* OwnerActor = GetOwner();
	if (IsValid(OwnerActor) == false) return FilterDisabledFeatures(CharacterInputState);
	if (OwnerActor->GetLocalRole() == ROLE_SimulatedProxy) return FilterDisabledFeatures(CharacterInputState);

	const UMotionMatchingCharacterMovementComponent* MotionMatchingMovement = GetOwnerMotionMatchingMovement();
	if (IsValid(MotionMatchingMovement) == false) return FilterDisabledFeatures(CharacterInputState);

	return FilterDisabledFeatures(MotionMatchingMovement->GetMovementInputState());
}


void UMotionMatchingComponent::UpdateCharacterInputState(const FMotionMatchingInputState& NewCharacterInputState)
{
	CharacterInputState = NewCharacterInputState;
	ServerRPC_UpdateInputState(CharacterInputState);

	// [추가] A-2: 이동 꾸러미에 실릴 입력 상태도 즉시 갱신
	UMotionMatchingCharacterMovementComponent* MotionMatchingMovement = GetOwnerMotionMatchingMovement();
	if (IsValid(MotionMatchingMovement) == false) return;

	MotionMatchingMovement->SetMovementInputState(CharacterInputState);
}


// [변경] PC 전용: 스틱 기울기 무관하게 항상 길이 1로 맞춘다
void UMotionMatchingComponent::ApplyMoveInput(const FVector2D& InputValue)
{
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (IsValid(OwnerCharacter) == false) return;

	const FVector2D ScaleValue = InputValue.GetSafeNormal();
	const FRotator YawRotation(0.0f, OwnerCharacter->GetControlRotation().Yaw, 0.0f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	OwnerCharacter->AddMovementInput(RightDirection, ScaleValue.X);
	OwnerCharacter->AddMovementInput(ForwardDirection, ScaleValue.Y);
}


// 카메라와 상관없이 월드 축(X = 앞, Y = 오른쪽) 기준으로 이동
void UMotionMatchingComponent::ApplyMoveWorldSpaceInput(const FVector2D& InputValue)
{
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (IsValid(OwnerCharacter) == false) return;

	const FVector2D ScaleValue = InputValue.GetSafeNormal();

	OwnerCharacter->AddMovementInput(FVector::RightVector, ScaleValue.X);
	OwnerCharacter->AddMovementInput(FVector::ForwardVector, ScaleValue.Y);
}


void UMotionMatchingComponent::SetWantsToSprint(bool bIsWantsToSprint)
{
	if (bIsWantsToSprint == true && bIsSprintEnabled == false) return; // [추가] v0.0.6

	FMotionMatchingInputState NewCharacterInputState = CharacterInputState;
	NewCharacterInputState.bIsWantsToSprint = bIsWantsToSprint;
	NewCharacterInputState.bIsWantsToWalk = false;
	UpdateCharacterInputState(NewCharacterInputState);
}


void UMotionMatchingComponent::ToggleWantsToWalk()
{
	if (bIsWalkToggleEnabled == false) return; // [추가] v0.0.6
	if (CharacterInputState.bIsWantsToSprint == true) return;

	FMotionMatchingInputState NewCharacterInputState = CharacterInputState;
	NewCharacterInputState.bIsWantsToWalk = (CharacterInputState.bIsWantsToWalk == false);
	UpdateCharacterInputState(NewCharacterInputState);
}


void UMotionMatchingComponent::ToggleWantsToStrafe()
{
	if (bIsStrafeEnabled == false) return; // [추가] v0.0.6

	FMotionMatchingInputState NewCharacterInputState = CharacterInputState;
	NewCharacterInputState.bIsWantsToStrafe = (CharacterInputState.bIsWantsToStrafe == false);
	UpdateCharacterInputState(NewCharacterInputState);
}


void UMotionMatchingComponent::SetWantsToAim(bool bIsWantsToAim)
{
	if (bIsWantsToAim == true && bIsAimEnabled == false) return; // [추가] v0.0.6

	FMotionMatchingInputState NewCharacterInputState = CharacterInputState;
	NewCharacterInputState.bIsWantsToAim = bIsWantsToAim;
	UpdateCharacterInputState(NewCharacterInputState);
}


void UMotionMatchingComponent::ToggleCrouch()
{
	if (bIsCrouchEnabled == false) return; // [추가] v0.0.6

	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (IsValid(OwnerCharacter) == false) return;

	const UCharacterMovementComponent* CharacterMovementComponent = OwnerCharacter->GetCharacterMovement();
	if (IsValid(CharacterMovementComponent) == false) return;
	if (CharacterMovementComponent->IsFalling() == true) return;

	if (CharacterMovementComponent->IsCrouching() == true)
	{
		OwnerCharacter->UnCrouch();
		return;
	}

	OwnerCharacter->Crouch();
}


void UMotionMatchingComponent::StartJumpInput()
{
	if (TryTraversalFromInput() == true) return;
	if (bIsJumpEnabled == false) return; // [추가] v0.0.6

	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (IsValid(OwnerCharacter) == false) return;

	OwnerCharacter->Jump();
}


void UMotionMatchingComponent::HoldJumpInput()
{
	TryTraversalFromInput();
}


// [추가] 기존 Character::Landed 역할 (다른 클라이언트 화면 캐릭터는 HandleOwnerMovementUpdated에서 처리)
void UMotionMatchingComponent::HandleOwnerLanded(const FHitResult& Hit)
{
	const ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (IsValid(OwnerCharacter) == false) return;
	if (OwnerCharacter->GetLocalRole() == ROLE_SimulatedProxy) return;

	HandleLandedMovement(OwnerCharacter->GetVelocity());
}


// [추가] 기존 OnJumped / OnWalkingOffLedge 역할: 걷다가 공중으로 바뀐 순간, 점프 키가 눌려 있으면 점프, 아니면 턱에서 떨어짐
void UMotionMatchingComponent::HandleOwnerMovementModeChanged(ACharacter* Character, EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	if (IsValid(Character) == false) return;
	if (Character->GetLocalRole() == ROLE_SimulatedProxy) return;
	if (PrevMovementMode != MOVE_Walking) return;

	const UCharacterMovementComponent* CharacterMovementComponent = Character->GetCharacterMovement();
	if (IsValid(CharacterMovementComponent) == false) return;
	if (CharacterMovementComponent->MovementMode != MOVE_Falling) return;

	if (Character->bPressedJump == true)
	{
		HandleJumpedMovement(Character->GetVelocity().Size2D());
		return;
	}

	Character->UnCrouch();
}


// 다른 클라이언트 화면 전용: 지난 프레임과 지면 상태가 바뀌었으면 착지/점프로 처리
void UMotionMatchingComponent::HandleOwnerMovementUpdated(float DeltaSeconds, FVector OldLocation, FVector OldVelocity)
{
	const ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (IsValid(OwnerCharacter) == false) return;

	const UCharacterMovementComponent* CharacterMovementComponent = OwnerCharacter->GetCharacterMovement();
	if (IsValid(CharacterMovementComponent) == false) return;

	const bool bIsMovingOnGround = CharacterMovementComponent->IsMovingOnGround();
	if (bIsMovingOnGround != bIsWasMovingOnGroundLastFrameSimulated)
	{
		if (bIsMovingOnGround == true)
		{
			HandleLandedMovement(OldVelocity);
		}
		else
		{
			HandleJumpedMovement(OldVelocity.Size2D());
		}
	}

	bIsWasMovingOnGroundLastFrameSimulated = bIsMovingOnGround;
}


void UMotionMatchingComponent::HandleLandedMovement(const FVector& NewLandVelocity)
{
	LandVelocity = NewLandVelocity;
	HandleMovementLanded(LandVelocity);

	// 착지 직후 JustLandedDuration 동안만 bIsJustLanded = true (애님에서 착지 모션 판정용)
	bIsJustLanded = true;
	GetWorld()->GetTimerManager().SetTimer(JustLandedTimerHandle, this, &ThisClass::ResetJustLanded, JustLandedDuration, false);
}


void UMotionMatchingComponent::HandleJumpedMovement(float GroundSpeedBeforeJump)
{
	HandleMovementJumped(GroundSpeedBeforeJump);
}


void UMotionMatchingComponent::ResetJustLanded()
{
	bIsJustLanded = false;
}

#pragma endregion


#pragma region Traversal
// BP TryTraversalFromInput: 진행 중이면 true, 아니면 검사 후 성공 여부 반환
bool UMotionMatchingComponent::TryTraversalFromInput()
{
	if (bIsDoingTraversalAction == true) return true;
	if (bIsTraversalEnabled == false) return false; // [추가] v0.0.6

	return TryTraversalAction(GetTraversalCheckInputs(), EDrawDebugTrace::ForDuration);
}


// BP GetTraversalCheckInputs: 공중이면 짧고 위쪽, 땅이면 속도에 비례해 멀리 검사
FMotionMatchingTraversalCheckInputs UMotionMatchingComponent::GetTraversalCheckInputs() const
{
	FMotionMatchingTraversalCheckInputs CheckInputs;

	const ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (IsValid(OwnerCharacter) == false) return CheckInputs;

	const UCharacterMovementComponent* CharacterMovementComponent = OwnerCharacter->GetCharacterMovement();
	if (IsValid(CharacterMovementComponent) == false) return CheckInputs;

	CheckInputs.TraceForwardDirection = OwnerCharacter->GetActorForwardVector();
	CheckInputs.TraceOriginOffset = FVector::ZeroVector;
	CheckInputs.TraceRadius = 30.0f;

	const EMovementMode MovementMode = CharacterMovementComponent->MovementMode;
	if (MovementMode == MOVE_Falling || MovementMode == MOVE_Flying)
	{
		CheckInputs.TraceForwardDistance = 75.0f;
		CheckInputs.TraceEndOffset = FVector(0.0f, 0.0f, 50.0f);
		CheckInputs.TraceHalfHeight = 86.0f;
		return CheckInputs;
	}

	// 캐릭터 기준 앞쪽 속도(X)가 0 → 500일 때 검사 거리 75 → 350
	const FVector LocalVelocity = OwnerCharacter->GetActorRotation().UnrotateVector(CharacterMovementComponent->Velocity);
	CheckInputs.TraceForwardDistance = UKismetMathLibrary::MapRangeClamped(LocalVelocity.X, 0.0f, 500.0f, 75.0f, 350.0f);
	CheckInputs.TraceEndOffset = FVector::ZeroVector;
	CheckInputs.TraceHalfHeight = 60.0f;
	return CheckInputs;
}


// BP TryTraversalAction: 장애물 검사 → Chooser → MotionMatch → 실행
// [추가] A-6
bool UMotionMatchingComponent::CanTraverse() const
{
	if (bIsDoingTraversalAction == true) return false;
	if (bIsTraversalEnabled == false) return false; // [추가] v0.0.6

	FMotionMatchingTraversalCheckResult CheckResult;
	TArray<UObject*> ValidMontages;
	return CheckTraversal(GetTraversalCheckInputs(), EDrawDebugTrace::None, CheckResult, ValidMontages);
}


// [변경] A-6: 기존 TryTraversalAction의 검사 부분(Step 1 ~ 5.1)을 분리. 상태를 바꾸지 않는다
bool UMotionMatchingComponent::CheckTraversal(const FMotionMatchingTraversalCheckInputs& CheckInputs, EDrawDebugTrace::Type DebugType, FMotionMatchingTraversalCheckResult& CheckResult, TArray<UObject*>& ValidMontages) const
{
	CheckResult = FMotionMatchingTraversalCheckResult();
	ValidMontages.Reset();

	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (IsValid(OwnerCharacter) == false) return false;

	UCharacterMovementComponent* CharacterMovementComponent = OwnerCharacter->GetCharacterMovement();
	if (IsValid(CharacterMovementComponent) == false) return false;

	const UCapsuleComponent* CapsuleComponent = OwnerCharacter->GetCapsuleComponent();
	if (IsValid(CapsuleComponent) == false) return false;

	// Step 1: 자주 쓰는 값 캐싱
	const FVector ActorLocation = OwnerCharacter->GetActorLocation();
	const float CapsuleRadius = CapsuleComponent->GetScaledCapsuleRadius();
	const float CapsuleHalfHeight = CapsuleComponent->GetScaledCapsuleHalfHeight();
	const int32 DrawDebugLevel = MotionMatchingTraversal::GetDrawDebugLevel();
	const float DrawDebugDuration = MotionMatchingTraversal::GetDrawDebugDuration();

	EDrawDebugTrace::Type ForwardTraceDebugType = EDrawDebugTrace::None;
	if (DrawDebugLevel >= 2)
	{
		ForwardTraceDebugType = DebugType;
	}

	EDrawDebugTrace::Type DetailTraceDebugType = EDrawDebugTrace::None;
	if (DrawDebugLevel >= 3)
	{
		DetailTraceDebugType = DebugType;
	}

	const ETraceTypeQuery TraversableTraceType = UEngineTypes::ConvertToTraceType(MotionMatchingCollision::Traversable);
	const ETraceTypeQuery VisibilityTraceType = UEngineTypes::ConvertToTraceType(ECC_Visibility);
	const TArray<AActor*> ActorsToIgnore;

	// Step 2.1: 앞쪽으로 캡슐 트레이스해서 Traversable 블록 찾기
	const FVector ForwardTraceStart = ActorLocation + CheckInputs.TraceOriginOffset;
	const FVector ForwardTraceEnd = ForwardTraceStart + (CheckInputs.TraceForwardDirection * CheckInputs.TraceForwardDistance) + CheckInputs.TraceEndOffset;
	FHitResult ForwardHitResult;
	UKismetSystemLibrary::CapsuleTraceSingle(OwnerCharacter, ForwardTraceStart, ForwardTraceEnd, CheckInputs.TraceRadius, CheckInputs.TraceHalfHeight, TraversableTraceType, false, ActorsToIgnore, ForwardTraceDebugType, ForwardHitResult, true, FLinearColor::Black, FLinearColor::Black, DrawDebugDuration);
	if (ForwardHitResult.bBlockingHit == false) return false;

	// Step 2.2: 블록에게 앞/뒤 턱 위치를 계산시킨다
	const AMotionMatchingTraversableBlock* TraversableBlock = Cast<AMotionMatchingTraversableBlock>(ForwardHitResult.GetActor());
	if (IsValid(TraversableBlock) == false) return false;

	const FMotionMatchingLedgeResult LedgeResult = TraversableBlock->ComputeLedgeTransforms(ForwardHitResult.ImpactPoint, ActorLocation);
	CheckResult.HitComponent = ForwardHitResult.GetComponent();
	CheckResult.bIsFrontLedgeFound = LedgeResult.bIsFrontLedgeFound;
	CheckResult.FrontLedgeLocation = LedgeResult.FrontLedgeLocation;
	CheckResult.FrontLedgeNormal = LedgeResult.FrontLedgeNormal;
	CheckResult.bIsBackLedgeFound = LedgeResult.bIsBackLedgeFound;
	CheckResult.BackLedgeLocation = LedgeResult.BackLedgeLocation;
	CheckResult.BackLedgeNormal = LedgeResult.BackLedgeNormal;

	// DEBUG: 앞 턱(초록), 뒤 턱(파랑) 위치 표시
	if (DrawDebugLevel >= 1 && DebugType != EDrawDebugTrace::None && CheckResult.bIsFrontLedgeFound == true) // [변경] A-6: 검사만 할 때(CanTraverse)는 표시 안 함
	{
		UKismetSystemLibrary::DrawDebugSphere(OwnerCharacter, CheckResult.FrontLedgeLocation, 10.0f, 12, FLinearColor::Green, DrawDebugDuration, 1.0f);
		if (CheckResult.bIsBackLedgeFound == true)
		{
			UKismetSystemLibrary::DrawDebugSphere(OwnerCharacter, CheckResult.BackLedgeLocation, 10.0f, 12, FLinearColor(0.0f, 0.639368f, 1.0f, 1.0f), DrawDebugDuration, 1.0f);
		}
	}

	// Step 3.1: 앞 턱이 없으면 종료
	if (CheckResult.bIsFrontLedgeFound == false) return false;

	// Step 3.2: 캐릭터 위치 → 앞 턱 위까지 올라갈 공간이 있는지 검사
	const FVector HasRoomCheckFrontLedgeLocation = CheckResult.FrontLedgeLocation + (CheckResult.FrontLedgeNormal * (CapsuleRadius + 2.0f)) + FVector(0.0f, 0.0f, CapsuleHalfHeight + 2.0f);
	FHitResult FrontRoomHitResult;
	UKismetSystemLibrary::CapsuleTraceSingle(OwnerCharacter, ActorLocation, HasRoomCheckFrontLedgeLocation, CapsuleRadius, CapsuleHalfHeight, VisibilityTraceType, false, ActorsToIgnore, DetailTraceDebugType, FrontRoomHitResult, true, FLinearColor::Red, FLinearColor::Green, DrawDebugDuration);
	if (FrontRoomHitResult.bBlockingHit == true || FrontRoomHitResult.bStartPenetrating == true)
	{
		CheckResult.bIsFrontLedgeFound = false;
		return false;
	}

	// Step 3.3: 장애물 높이 = 캐릭터 발 위치와 앞 턱의 높이 차
	CheckResult.ObstacleHeight = FMath::Abs(((ActorLocation - FVector(0.0f, 0.0f, CapsuleHalfHeight)) - CheckResult.FrontLedgeLocation).Z);

	// Step 3.4: 앞 턱 → 뒤 턱 위쪽을 가로질러 공간이 있는지 검사
	const FVector BackLedgeOffsetLocation = CheckResult.BackLedgeLocation + (CheckResult.BackLedgeNormal * (CapsuleRadius + 2.0f));
	const FVector HasRoomCheckBackLedgeLocation = BackLedgeOffsetLocation + FVector(0.0f, 0.0f, CapsuleHalfHeight + 2.0f);
	FHitResult TopSweepHitResult;
	const bool bIsTopSweepHit = UKismetSystemLibrary::CapsuleTraceSingle(OwnerCharacter, HasRoomCheckFrontLedgeLocation, HasRoomCheckBackLedgeLocation, CapsuleRadius, CapsuleHalfHeight, VisibilityTraceType, false, ActorsToIgnore, DetailTraceDebugType, TopSweepHitResult, true, FLinearColor::Red, FLinearColor::Green, DrawDebugDuration);
	if (bIsTopSweepHit == false)
	{
		// Step 3.5: 공간이 있으면 깊이 = 앞/뒤 턱 사이 수평 거리
		CheckResult.ObstacleDepth = (CheckResult.FrontLedgeLocation - CheckResult.BackLedgeLocation).Size2D();

		// Step 3.6: 뒤 턱 바깥에서 아래로 바닥 찾기 (현재 BP와 동일하게 50 아래까지)
		const FVector FloorTraceEnd = BackLedgeOffsetLocation - FVector(0.0f, 0.0f, 50.0f);
		FHitResult FloorHitResult;
		UKismetSystemLibrary::CapsuleTraceSingle(OwnerCharacter, HasRoomCheckBackLedgeLocation, FloorTraceEnd, CapsuleRadius, CapsuleHalfHeight, VisibilityTraceType, false, ActorsToIgnore, DetailTraceDebugType, FloorHitResult, true, FLinearColor::Red, FLinearColor::Green, DrawDebugDuration);
		CheckResult.bIsBackFloorFound = FloorHitResult.bBlockingHit;
		if (FloorHitResult.bBlockingHit == true)
		{
			CheckResult.BackFloorLocation = FloorHitResult.ImpactPoint;
			CheckResult.BackLedgeHeight = FMath::Abs((FloorHitResult.ImpactPoint - CheckResult.BackLedgeLocation).Z);
		}
	}
	else
	{
		// Step 3.5: 공간이 없으면 깊이 = 앞 턱 ~ 막힌 지점, 뒤 턱은 무효
		CheckResult.ObstacleDepth = (TopSweepHitResult.ImpactPoint - CheckResult.FrontLedgeLocation).Size2D();
		CheckResult.bIsBackLedgeFound = false;
	}

	// Step 5.3: Chooser로 조건에 맞는 몽타주 후보와 ActionType을 고른다
	if (IsValid(TraversalAnimsChooserTable) == false)
	{
		UE_LOG(LogMotionMatching, Error, TEXT("[MotionMatching] TraversalAnimsChooserTable이 비어 있습니다. 캐릭터 BP의 MotionMatchingComponent Details에서 CHT_TraversalAnims를 지정하세요."));
		return false;
	}

	FMotionMatchingTraversalChooserInputs ChooserInputs;
	ChooserInputs.ActionType = CheckResult.ActionType;
	ChooserInputs.bIsFrontLedgeFound = CheckResult.bIsFrontLedgeFound;
	ChooserInputs.bIsBackLedgeFound = CheckResult.bIsBackLedgeFound;
	ChooserInputs.bIsBackFloorFound = CheckResult.bIsBackFloorFound;
	ChooserInputs.ObstacleHeight = CheckResult.ObstacleHeight;
	ChooserInputs.ObstacleDepth = CheckResult.ObstacleDepth;
	ChooserInputs.BackLedgeHeight = CheckResult.BackLedgeHeight;
	ChooserInputs.MovementMode = CharacterMovementComponent->MovementMode;
	ChooserInputs.Speed = CharacterMovementComponent->Velocity.Size2D();

	ChooserInputs.Gait = Gait; // [변경] 캐릭터 대신 컴포넌트의 Gait 사용

	FMotionMatchingTraversalChooserOutputs ChooserOutputs;
	FChooserEvaluationContext ChooserContext;
	ChooserContext.AddStructParam(ChooserInputs);
	ChooserContext.AddStructParam(ChooserOutputs);

	const FInstancedStruct ChooserInstance = UChooserFunctionLibrary::MakeEvaluateChooser(TraversalAnimsChooserTable);
	ValidMontages = UChooserFunctionLibrary::EvaluateObjectChooserBaseMulti(ChooserContext, ChooserInstance, UAnimMontage::StaticClass());
	CheckResult.ActionType = ChooserOutputs.ActionType;

	// DEBUG: 검사 결과 출력
	if (DrawDebugLevel >= 1 && DebugType != EDrawDebugTrace::None) // [변경] A-6
	{
		FString DebugString = TEXT("Has Front Ledge: ") + UKismetStringLibrary::Conv_BoolToString(CheckResult.bIsFrontLedgeFound);
		DebugString += TEXT("\r\nHas Back Ledge: ") + UKismetStringLibrary::Conv_BoolToString(CheckResult.bIsBackLedgeFound);
		DebugString += TEXT("\r\nHas Back Floor:") + UKismetStringLibrary::Conv_BoolToString(CheckResult.bIsBackFloorFound);
		DebugString += TEXT("\r\nObstacle Height: ") + UKismetStringLibrary::Conv_DoubleToString(CheckResult.ObstacleHeight);
		DebugString += TEXT("\r\nObstacle Depth: ") + UKismetStringLibrary::Conv_DoubleToString(CheckResult.ObstacleDepth);
		DebugString += TEXT("\r\nBack Ledge Height: ") + UKismetStringLibrary::Conv_DoubleToString(CheckResult.BackLedgeHeight);
		UKismetSystemLibrary::PrintString(OwnerCharacter, DebugString, true, false, FLinearColor(0.0f, 0.66f, 1.0f, 1.0f), 2.0f, TEXT("Traversal 1"));

		const FString ActionTypeString = UEnum::GetDisplayValueAsText(CheckResult.ActionType).ToString();
		UKismetSystemLibrary::PrintString(OwnerCharacter, ActionTypeString, true, false, FLinearColor(1.0f, 0.0f, 0.824021f, 1.0f), 2.0f, TEXT("Traversal 2"));
	}

	// Step 5.1: 할 수 있는 동작이 없으면 종료
	if (CheckResult.ActionType == EMotionMatchingTraversalActionType::None) return false;

	return true;
}


bool UMotionMatchingComponent::TryTraversalAction(const FMotionMatchingTraversalCheckInputs& CheckInputs, EDrawDebugTrace::Type DebugType)
{
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (IsValid(OwnerCharacter) == false) return false;

	// [변경] A-6: 검사는 CheckTraversal에서 수행
	FMotionMatchingTraversalCheckResult CheckResult;
	TArray<UObject*> ValidMontages;
	if (CheckTraversal(CheckInputs, DebugType, CheckResult, ValidMontages) == false) return false;

	// Step 5.2: 앞 턱 Transform을 ABP에 전달 (MotionMatch의 Interaction 채널이 사용)
	UAnimInstance* AnimInstance = GetOwnerAnimInstance();
	if (IsValid(AnimInstance) == false) return false;

	const FTransform InteractionTransform(UKismetMathLibrary::MakeRotFromZ(CheckResult.FrontLedgeNormal), CheckResult.FrontLedgeLocation, FVector::OneVector);
	if (SetInteractionTransformOnAnimInstance(AnimInstance, InteractionTransform) == false) return false;

	// Step 5.4: 후보 몽타주 중 현재 포즈와 가장 잘 맞는 몽타주와 시작 시간을 고른다
	FPoseSearchBlueprintResult MotionMatchResult;
	UPoseSearchLibrary::MotionMatch(AnimInstance, ValidMontages, PoseHistoryName, FPoseSearchContinuingProperties(), FPoseSearchFutureProperties(), MotionMatchResult);

	UAnimMontage* SelectedMontage = Cast<UAnimMontage>(MotionMatchResult.SelectedAnim);
	if (IsValid(SelectedMontage) == false)
	{
		UKismetSystemLibrary::PrintString(OwnerCharacter, TEXT("Failed To Find Montage!"), true, false, FLinearColor::Red, 2.0f);
		return false;
	}

	CheckResult.ChosenMontage = SelectedMontage;
	CheckResult.StartTime = MotionMatchResult.SelectedTime;
	CheckResult.PlayRate = MotionMatchResult.WantedPlayRate;

	// Step 5.5: 로컬에서 바로 실행하고, 서버/다른 클라이언트에 알린다
	TraversalResult = CheckResult;
	PerformTraversalAction();

	if (GetOwnerRole() == ROLE_Authority) return true;

	ServerRPC_PerformTraversal(TraversalResult);
	return true;
}


// BP PerformTraversalAction: 워프 타겟 갱신 → 보정 끄기 → 몽타주 재생 → Flying
void UMotionMatchingComponent::PerformTraversalAction()
{
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (IsValid(OwnerCharacter) == false) return;

	UCharacterMovementComponent* CharacterMovementComponent = OwnerCharacter->GetCharacterMovement();
	if (IsValid(CharacterMovementComponent) == false) return;

	UCapsuleComponent* CapsuleComponent = OwnerCharacter->GetCapsuleComponent();
	if (IsValid(CapsuleComponent) == false) return;

	UAnimInstance* AnimInstance = GetOwnerAnimInstance();
	if (IsValid(AnimInstance) == false) return;

	UpdateWarpTargets();
	OnTraversalStart();

	const float PlayedLength = AnimInstance->Montage_Play(TraversalResult.ChosenMontage, TraversalResult.PlayRate, EMontagePlayReturnType::MontageLength, TraversalResult.StartTime, true);
	if (PlayedLength > 0.0f)
	{
		FOnMontageBlendingOutStarted BlendingOutDelegate = FOnMontageBlendingOutStarted::CreateUObject(this, &ThisClass::HandleTraversalMontageBlendingOut);
		AnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, TraversalResult.ChosenMontage);

		FOnMontageEnded EndedDelegate = FOnMontageEnded::CreateUObject(this, &ThisClass::HandleTraversalMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndedDelegate, TraversalResult.ChosenMontage);
	}

	bIsDoingTraversalAction = true;
	CapsuleComponent->IgnoreComponentWhenMoving(TraversalResult.HitComponent, true);
	CharacterMovementComponent->SetMovementMode(MOVE_Flying);
	OnTraversalStarted.Broadcast(TraversalResult.ActionType); // [추가] A-6

	// 몽타주 재생 실패 시 바로 원상복구 (BP는 이 경우 Flying에 멈출 수 있음)
	if (PlayedLength > 0.0f) return;

	UE_LOG(LogMotionMatching, Warning, TEXT("[MotionMatching] 트래버설 몽타주 재생에 실패했습니다: %s"), *GetNameSafe(TraversalResult.ChosenMontage));
	FinishTraversalAction();
}


// BP UpdateWarpTargets: FrontLedge는 항상, BackLedge는 Hurdle/Vault, BackFloor는 Hurdle만
void UMotionMatchingComponent::UpdateWarpTargets()
{
	UMotionWarpingComponent* MotionWarpingComponent = GetOwnerMotionWarpingComponent();
	if (IsValid(MotionWarpingComponent) == false)
	{
		UE_LOG(LogMotionMatching, Error, TEXT("[MotionMatching] 캐릭터에 MotionWarping 컴포넌트가 없습니다."));
		return;
	}

	const FVector FrontLedgeWarpLocation = TraversalResult.FrontLedgeLocation + FVector(0.0f, 0.0f, 0.5f);
	const FRotator FrontLedgeWarpRotation = UKismetMathLibrary::MakeRotFromX(TraversalResult.FrontLedgeNormal * -1.0f);
	MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(MotionMatchingTraversal::FrontLedgeWarpTargetName, FrontLedgeWarpLocation, FrontLedgeWarpRotation);

	float AnimatedDistanceFromFrontLedgeToBackLedge = 0.0f;
	bool bIsBackLedgeWarpTargetSet = false;
	if (TraversalResult.ActionType == EMotionMatchingTraversalActionType::Hurdle || TraversalResult.ActionType == EMotionMatchingTraversalActionType::Vault)
	{
		TArray<FMotionWarpingWindowData> BackLedgeWindows;
		UMotionWarpingUtilities::GetMotionWarpingWindowsForWarpTargetFromAnimation(TraversalResult.ChosenMontage, MotionMatchingTraversal::BackLedgeWarpTargetName, BackLedgeWindows);
		if (BackLedgeWindows.IsEmpty() == false)
		{
			UAnimationWarpingLibrary::GetCurveValueFromAnimation(TraversalResult.ChosenMontage, MotionMatchingTraversal::DistanceFromLedgeCurveName, BackLedgeWindows[0].EndTime, AnimatedDistanceFromFrontLedgeToBackLedge);
			MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(MotionMatchingTraversal::BackLedgeWarpTargetName, TraversalResult.BackLedgeLocation, FRotator::ZeroRotator);
			bIsBackLedgeWarpTargetSet = true;
		}
	}

	if (bIsBackLedgeWarpTargetSet == false)
	{
		MotionWarpingComponent->RemoveWarpTarget(MotionMatchingTraversal::BackLedgeWarpTargetName);
	}

	bool bIsBackFloorWarpTargetSet = false;
	if (TraversalResult.ActionType == EMotionMatchingTraversalActionType::Hurdle)
	{
		TArray<FMotionWarpingWindowData> BackFloorWindows;
		UMotionWarpingUtilities::GetMotionWarpingWindowsForWarpTargetFromAnimation(TraversalResult.ChosenMontage, MotionMatchingTraversal::BackFloorWarpTargetName, BackFloorWindows);
		if (BackFloorWindows.IsEmpty() == false)
		{
			float AnimatedDistanceFromFrontLedgeToBackFloor = 0.0f;
			UAnimationWarpingLibrary::GetCurveValueFromAnimation(TraversalResult.ChosenMontage, MotionMatchingTraversal::DistanceFromLedgeCurveName, BackFloorWindows[0].EndTime, AnimatedDistanceFromFrontLedgeToBackFloor);

			// 애니메이션 속 뒤 턱 → 착지 거리만큼 뒤 턱에서 바깥으로, 높이는 실제 바닥
			const float BackLedgeToBackFloorDistance = FMath::Abs(AnimatedDistanceFromFrontLedgeToBackLedge - AnimatedDistanceFromFrontLedgeToBackFloor);
			const FVector BackFloorHorizontalLocation = TraversalResult.BackLedgeLocation + (TraversalResult.BackLedgeNormal * BackLedgeToBackFloorDistance);
			const FVector BackFloorWarpLocation(BackFloorHorizontalLocation.X, BackFloorHorizontalLocation.Y, TraversalResult.BackFloorLocation.Z);
			MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(MotionMatchingTraversal::BackFloorWarpTargetName, BackFloorWarpLocation, FRotator::ZeroRotator);
			bIsBackFloorWarpTargetSet = true;
		}
	}

	if (bIsBackFloorWarpTargetSet == false)
	{
		MotionWarpingComponent->RemoveWarpTarget(MotionMatchingTraversal::BackFloorWarpTargetName);
	}
}


// BP OnTraversalStart: 트래버설 동안 클라이언트 위치를 서버가 그대로 받아들인다
void UMotionMatchingComponent::OnTraversalStart()
{
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (IsValid(OwnerCharacter) == false) return;

	UCharacterMovementComponent* CharacterMovementComponent = OwnerCharacter->GetCharacterMovement();
	if (IsValid(CharacterMovementComponent) == false) return;

	// [추가] 이전 트래버설의 '보정 다시 켜기' 예약이 이번 트래버설 도중에 실행되지 않도록 취소
	GetWorld()->GetTimerManager().ClearTimer(IgnoreCorrectionsTimerHandle);

	CharacterMovementComponent->bIgnoreClientMovementErrorChecksAndCorrection = true;
	CharacterMovementComponent->bServerAcceptClientAuthoritativePosition = true;
}


// BP OnTraversalEnd: 서버 위치 보정을 다시 켠다
void UMotionMatchingComponent::OnTraversalEnd()
{
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (IsValid(OwnerCharacter) == false) return;

	UCharacterMovementComponent* CharacterMovementComponent = OwnerCharacter->GetCharacterMovement();
	if (IsValid(CharacterMovementComponent) == false) return;

	CharacterMovementComponent->bIgnoreClientMovementErrorChecksAndCorrection = false;
	CharacterMovementComponent->bServerAcceptClientAuthoritativePosition = false;

	// [추가] 안전장치: 트래버설이 끝났는데 아직 비행 상태면 낙하로 돌려 바닥에서 자연스럽게 걷기로 전환
	if (bIsDoingTraversalAction == true) return;
	if (CharacterMovementComponent->MovementMode != MOVE_Flying) return;

	CharacterMovementComponent->SetMovementMode(MOVE_Falling);
}


// BP 몽타주 OnCompleted/OnInterrupted 이후 흐름: 상태 해제 → 이동 모드 복구 → 지연 후 보정 재개
void UMotionMatchingComponent::FinishTraversalAction()
{
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (IsValid(OwnerCharacter) == false) return;

	UCharacterMovementComponent* CharacterMovementComponent = OwnerCharacter->GetCharacterMovement();
	if (IsValid(CharacterMovementComponent) == false) return;

	UCapsuleComponent* CapsuleComponent = OwnerCharacter->GetCapsuleComponent();
	if (IsValid(CapsuleComponent) == false) return;

	bIsDoingTraversalAction = false;
	CapsuleComponent->IgnoreComponentWhenMoving(TraversalResult.HitComponent, false);

	// Vault는 넘어간 뒤 떨어지므로 Falling, 나머지는 Walking
	if (TraversalResult.ActionType == EMotionMatchingTraversalActionType::Vault)
	{
		CharacterMovementComponent->SetMovementMode(MOVE_Falling);
	}
	else
	{
		CharacterMovementComponent->SetMovementMode(MOVE_Walking);
	}

	OnTraversalFinished.Broadcast(TraversalResult.ActionType); // [추가] A-6

	// [변경] 클라이언트는 끝났음을 서버에 알린다. 서버는 양쪽이 모두 끝난 뒤 위치 보정을 다시 켠다
	if (OwnerCharacter->GetLocalRole() == ROLE_AutonomousProxy)
	{
		ServerRPC_FinishTraversal();
	}

	ScheduleTraversalEnd();
}


// [추가] 위치 보정 다시 켜기 예약. 서버는 클라이언트 종료 알림이 오기 전이면 최대 ClientFinishWaitTimeout까지 기다린다
void UMotionMatchingComponent::ScheduleTraversalEnd()
{
	if (bIsDoingTraversalAction == true) return;

	float Delay = IgnoreCorrectionsDelay;
	if (bIsClientTraversalFinished == false)
	{
		Delay = ClientFinishWaitTimeout;
	}

	if (Delay <= 0.0f)
	{
		OnTraversalEnd();
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(IgnoreCorrectionsTimerHandle, this, &ThisClass::OnTraversalEnd, Delay, false);
}


void UMotionMatchingComponent::HandleTraversalMontageBlendingOut(UAnimMontage* Montage, bool bIsInterrupted)
{
	if (bIsInterrupted == false) return;

	FinishTraversalAction();
}


void UMotionMatchingComponent::HandleTraversalMontageEnded(UAnimMontage* Montage, bool bIsInterrupted)
{
	if (bIsInterrupted == true) return;

	FinishTraversalAction();
}


// BPI_InteractionTransform은 아직 BP 인터페이스라 이름으로 찾아서 호출한다 (플러그인화 때 C++ 인터페이스로 교체)
bool UMotionMatchingComponent::SetInteractionTransformOnAnimInstance(UAnimInstance* AnimInstance, const FTransform& InteractionTransform) const
{
	// [변경] A-5: 이름으로 함수를 찾던 방식(BPI_InteractionTransform) 대신 C++ ABP 부모 클래스에 직접 전달
	UMotionMatchingAnimInstance* MotionMatchingAnimInstance = Cast<UMotionMatchingAnimInstance>(AnimInstance);
	if (IsValid(MotionMatchingAnimInstance) == false)
	{
		UE_LOG(LogMotionMatching, Error, TEXT("[MotionMatching] %s의 부모 클래스가 UMotionMatchingAnimInstance가 아닙니다. ABP의 Class Settings에서 부모를 바꾸세요."), *GetNameSafe(AnimInstance->GetClass()));
		return false;
	}

	MotionMatchingAnimInstance->SetTraversalInteractionTransform(InteractionTransform);
	return true;
}
#pragma endregion


#pragma region Sound
// [추가] 점프: 알림 후, 점프 직전 속도로 볼륨을 정해 점프 소리 재생
void UMotionMatchingComponent::HandleMovementJumped(float GroundSpeedBeforeJump)
{
	OnMovementJumped.Broadcast(GroundSpeedBeforeJump);

	if (bIsDefaultSoundEnabled == false) return;
	if (IsValid(JumpSound) == false) return;

	const float VolumeMultiplier = UKismetMathLibrary::MapRangeClamped(GroundSpeedBeforeJump, JumpSpeedRangeForVolume.X, JumpSpeedRangeForVolume.Y, JumpVolumeRange.X, JumpVolumeRange.Y);
	PlaySoundAtOwnerMesh(JumpSound, VolumeMultiplier);
}


// [추가] 착지: 알림 후, 낙하 속도(Z)로 볼륨을 정해 착지 소리 재생
void UMotionMatchingComponent::HandleMovementLanded(const FVector& NewLandVelocity) // [변경] 멤버 LandVelocity와 이름 충돌(C4458) 방지
{
	OnMovementLanded.Broadcast(NewLandVelocity);

	if (bIsDefaultSoundEnabled == false) return;
	if (IsValid(LandSound) == false) return;

	const float VolumeMultiplier = UKismetMathLibrary::MapRangeClamped(NewLandVelocity.Z, LandFallSpeedRangeForVolume.X, LandFallSpeedRangeForVolume.Y, LandVolumeRange.X, LandVolumeRange.Y);
	PlaySoundAtOwnerMesh(LandSound, VolumeMultiplier);
}


// [추가] 캐릭터 메시 위치에서 소리 재생 (기존 BP PlayAudioEvent와 동일, Pitch 1.0)
void UMotionMatchingComponent::PlaySoundAtOwnerMesh(USoundBase* Sound, float VolumeMultiplier) const
{
	const ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (IsValid(OwnerCharacter) == false) return;

	const USkeletalMeshComponent* MeshComponent = OwnerCharacter->GetMesh();
	if (IsValid(MeshComponent) == false) return;

	UGameplayStatics::PlaySoundAtLocation(this, Sound, MeshComponent->GetComponentLocation(), VolumeMultiplier, 1.0f);
}


// [추가] A-5: 땅 위에 있거나 트래버설 중일 때만 걸음 소리 재생 (기존 CBP CanPlayFootstepSounds와 동일)
bool UMotionMatchingComponent::CanPlayFootstepSounds() const
{
	if (bIsDoingTraversalAction == true) return true;

	const ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (IsValid(OwnerCharacter) == false) return false;

	const UCharacterMovementComponent* CharacterMovementComponent = OwnerCharacter->GetCharacterMovement();
	if (IsValid(CharacterMovementComponent) == false) return false;

	return CharacterMovementComponent->IsMovingOnGround();
}


// [추가] A-5
UMotionMatchingComponent* UMotionMatchingComponent::FindMotionMatchingComponent(const AActor* Actor)
{
	if (IsValid(Actor) == false) return nullptr;

	return Actor->FindComponentByClass<UMotionMatchingComponent>();
}
#pragma endregion


#pragma region Network
// [추가] 캐릭터에서 옮겨옴
void UMotionMatchingComponent::ServerRPC_UpdateInputState_Implementation(const FMotionMatchingInputState& NewCharacterInputState)
{
	CharacterInputState = NewCharacterInputState;
}


// 클라이언트가 시작한 트래버설을 서버에서 한 번 실행하고, 다른 클라이언트에 복제
void UMotionMatchingComponent::ServerRPC_PerformTraversal_Implementation(const FMotionMatchingTraversalCheckResult& NewTraversalResult)
{
	bIsClientTraversalFinished = false; // [추가]
	TraversalResult = NewTraversalResult;
	PerformTraversalAction();
}


// [추가]
void UMotionMatchingComponent::ServerRPC_FinishTraversal_Implementation()
{
	bIsClientTraversalFinished = true;
	ScheduleTraversalEnd();
}


// 다른 클라이언트(SimulatedProxy)에서 같은 몽타주를 재생
void UMotionMatchingComponent::OnRep_TraversalResult()
{
	PerformTraversalAction();
}
#pragma endregion


#pragma region Helper
ACharacter* UMotionMatchingComponent::GetOwnerCharacter() const
{
	return Cast<ACharacter>(GetOwner());
}


UAnimInstance* UMotionMatchingComponent::GetOwnerAnimInstance() const
{
	const ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (IsValid(OwnerCharacter) == false) return nullptr;

	const USkeletalMeshComponent* MeshComponent = OwnerCharacter->GetMesh();
	if (IsValid(MeshComponent) == false) return nullptr;

	return MeshComponent->GetAnimInstance();
}


// [추가] A-2
UMotionMatchingCharacterMovementComponent* UMotionMatchingComponent::GetOwnerMotionMatchingMovement() const
{
	const ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (IsValid(OwnerCharacter) == false) return nullptr;

	return Cast<UMotionMatchingCharacterMovementComponent>(OwnerCharacter->GetCharacterMovement());
}


UMotionWarpingComponent* UMotionMatchingComponent::GetOwnerMotionWarpingComponent() const
{
	const AActor* OwnerActor = GetOwner();
	if (IsValid(OwnerActor) == false) return nullptr;

	return OwnerActor->FindComponentByClass<UMotionWarpingComponent>();
}
#pragma endregion
