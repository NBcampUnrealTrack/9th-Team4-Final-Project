#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Enum/MotionMatchingTypes.h"
#include "Engine/DataAsset.h" // [추가] A-5
#include "GameplayTagContainer.h" // [추가] v0.0.7
#include "MotionMatchingComponent.generated.h"

class ACharacter;
class UAnimInstance;
class UAnimMontage;
class UChooserTable;
class UMotionWarpingComponent;
class USoundBase; // [추가]
class UCurveFloat; // [추가]
class UCharacterMovementComponent; // [추가]
class UMotionMatchingCharacterMovementComponent; // [추가] A-2

// [추가] v0.0.6: 게임 중 켜고 끌 수 있는 부가 기능 (기본 이동·시점 회전은 제외)
UENUM(BlueprintType)
enum class EMotionMatchingFeature : uint8
{
	Sprint,
	WalkToggle,
	Strafe,
	Aim,
	Crouch,
	Jump,
	Traversal
};

// [추가] 점프/착지 알림 (BP에서 바인딩해서 직접 처리할 때 사용)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMotionMatchingMovementJumped, float, GroundSpeedBeforeJump);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMotionMatchingMovementLanded, FVector, LandVelocity);

// [추가] A-6: GA 등에서 상태 변화를 받기 위한 알림
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMotionMatchingGaitChanged, EMotionMatchingGait, NewGait, EMotionMatchingGait, PreviousGait);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMotionMatchingTraversalStarted, EMotionMatchingTraversalActionType, ActionType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMotionMatchingTraversalFinished, EMotionMatchingTraversalActionType, ActionType);

// [추가] 모션매칭 기능을 담는 단일 컴포넌트. 기능별로 #pragma region으로 구분한다
UCLASS(ClassGroup = (MotionMatching), meta = (BlueprintSpawnableComponent))
class PROJECT_NF_API UMotionMatchingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMotionMatchingComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override; // [추가] 캐릭터 이동 계산 직전에 실행

protected:
	virtual void BeginPlay() override; // [추가]
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override; // [추가] v0.0.7

#pragma region Feature
public:
	// [추가] v0.0.6: 부가 기능 켜기/끄기. 서버에서 호출하면 모든 화면에 적용, 조종 클라이언트에서 호출하면 서버로 전달. 끄는 순간 그 기능의 현재 상태도 풀린다
	UFUNCTION(BlueprintCallable, Category = "MotionMatching|Feature")
	void SetFeatureEnabled(EMotionMatchingFeature Feature, bool bIsEnabled);

	UFUNCTION(BlueprintPure, Category = "MotionMatching|Feature")
	bool IsFeatureEnabled(EMotionMatchingFeature Feature) const;

protected:
	void ApplyFeatureEnabled(EMotionMatchingFeature Feature, bool bIsEnabled);
	void ClearDisabledFeatureStates();
	FMotionMatchingInputState FilterDisabledFeatures(const FMotionMatchingInputState& InputState) const;

	UFUNCTION(Server, Reliable)
	void ServerRPC_SetFeatureEnabled(EMotionMatchingFeature Feature, bool bIsEnabled);

	UFUNCTION()
	void OnRep_FeatureFlags();

protected:
	// [추가] v0.0.6: 끄면 해당 기능을 쓸 수 없음 (Details에서 기본값 지정, 게임 중에는 SetFeatureEnabled 사용)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_FeatureFlags, Category = "MotionMatching|Feature")
	bool bIsSprintEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_FeatureFlags, Category = "MotionMatching|Feature")
	bool bIsWalkToggleEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_FeatureFlags, Category = "MotionMatching|Feature")
	bool bIsStrafeEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_FeatureFlags, Category = "MotionMatching|Feature")
	bool bIsAimEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_FeatureFlags, Category = "MotionMatching|Feature")
	bool bIsCrouchEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_FeatureFlags, Category = "MotionMatching|Feature")
	bool bIsJumpEnabled = true;

	// 끄면 점프 키를 눌러도 파쿠르 없이 일반 점프만 한다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_FeatureFlags, Category = "MotionMatching|Feature")
	bool bIsTraversalEnabled = true;
#pragma endregion

#pragma region Movement
public:
	// [추가] 파쿠르 중인데 몽타주 인스턴스가 사라졌으면 종료 처리를 대신 실행
	void CheckTraversalMontageLost();
	
	// [추가] 아래 함수들은 컨트롤러(입력)나 GA가 호출하는 행동 함수
	UFUNCTION(BlueprintCallable, Category = "MotionMatching|Movement")
	void ApplyMoveInput(const FVector2D& InputValue);

	UFUNCTION(BlueprintCallable, Category = "MotionMatching|Movement")
	void ApplyMoveWorldSpaceInput(const FVector2D& InputValue);

	UFUNCTION(BlueprintCallable, Category = "MotionMatching|Movement")
	void SetWantsToSprint(bool bIsWantsToSprint);

	UFUNCTION(BlueprintCallable, Category = "MotionMatching|Movement")
	void ToggleWantsToWalk();

	// [추가] 외부(줌 등)에서 걷기 강제 on/off. 끌 때는 강제 전 걷기 상태로 복원 (조종하는 클라이언트에서 호출)
	UFUNCTION(BlueprintCallable, Category = "MotionMatching|Movement")
	void SetForceWalk(bool bIsForceWalk);

	UFUNCTION(BlueprintCallable, Category = "MotionMatching|Movement")
	void ToggleWantsToStrafe();

	UFUNCTION(BlueprintCallable, Category = "MotionMatching|Movement")
	void SetWantsToAim(bool bIsWantsToAim);

	UFUNCTION(BlueprintCallable, Category = "MotionMatching|Movement")
	void ToggleCrouch();

	// [추가] 점프 키를 누른 순간: 트래버설을 먼저 시도하고 실패하면 점프
	UFUNCTION(BlueprintCallable, Category = "MotionMatching|Movement")
	void StartJumpInput();

	// [추가] 점프 키를 누르고 있는 동안: 트래버설만 계속 시도
	UFUNCTION(BlueprintCallable, Category = "MotionMatching|Movement")
	void HoldJumpInput();

	UFUNCTION(BlueprintPure, Category = "MotionMatching|Movement")
	EMotionMatchingGait GetGait() const { return Gait; }

	// [추가] 달리기 최고 속도 (X = 앞, Y = 옆, Z = 뒤). 질주 카메라 연출의 시작 지점으로 사용
	UFUNCTION(BlueprintPure, Category = "MotionMatching|Movement|Speed")
	FVector GetRunSpeeds() const { return RunSpeeds; }

	// [추가] 질주 최고 속도 (X = 앞, Y = 옆, Z = 뒤). 질주 카메라 연출의 최대 지점으로 사용
	UFUNCTION(BlueprintPure, Category = "MotionMatching|Movement|Speed")
	FVector GetSprintSpeeds() const { return SprintSpeeds; }

	UFUNCTION(BlueprintPure, Category = "MotionMatching|Movement")
	FMotionMatchingInputState GetCharacterInputState() const { return CharacterInputState; }

	UFUNCTION(BlueprintPure, Category = "MotionMatching|Movement")
	FVector GetLandVelocity() const { return LandVelocity; }

	UFUNCTION(BlueprintPure, Category = "MotionMatching|Movement")
	bool IsJustLanded() const { return bIsJustLanded; }

	// [추가] A-6: 이동 속도 배율 (GA·GE에서 조절, 1 = 기본, 0 미만은 0). 서버와 조종 클라이언트 양쪽에서 같은 값으로 호출하면 보정 없이 동작
	UFUNCTION(BlueprintCallable, Category = "MotionMatching|Movement|Speed")
	void SetMovementSpeedMultiplier(float NewMovementSpeedMultiplier);

	UFUNCTION(BlueprintPure, Category = "MotionMatching|Movement|Speed")
	float GetMovementSpeedMultiplier() const { return MovementSpeedMultiplier; }

public:
	// [추가] A-6: Gait(걷기/달리기/질주)가 바뀔 때 알림
	UPROPERTY(BlueprintAssignable, Category = "MotionMatching|Movement")
	FOnMotionMatchingGaitChanged OnGaitChanged;

	// [추가] A-2: 이동 컴포넌트(UMotionMatchingCharacterMovementComponent)가 이동 계산 중에 호출
	float CalculateMaxSpeed() const;
	float CalculateMaxCrouchSpeed() const;
	float CalculateMaxAcceleration() const;
	float CalculateBrakingDeceleration() const;
	float CalculateGroundFriction() const;

protected:
	// [추가] 캐릭터에서 옮겨온 이동 계산 (매 틱 CharacterMovement 직전에 실행)
	void UpdateRotation_PreCMC();
	void UpdateMovement_PreCMC();
	EMotionMatchingGait GetDesiredGait() const;
	bool CanSprint() const;
	float CalculateBrakingFriction() const;
	float CalculateStrafeSpeedMap(bool bIsStrafing) const;
	FVector GetGaitSpeeds() const;
	bool HasMovementInputVector() const;
	FMotionMatchingInputState GetMovementInputState() const; // [추가] A-2: 이동 계산에 쓸 입력 상태 (본인·서버는 이동 컴포넌트 값, 다른 클라 화면은 복제 값)
	void UpdateCharacterInputState(const FMotionMatchingInputState& NewCharacterInputState);
	void ResetJustLanded();

	// [추가] 착지·점프 감지 (캐릭터 함수를 덮어쓰지 않고 엔진 알림을 받아서 처리)
	UFUNCTION()
	void HandleOwnerLanded(const FHitResult& Hit);

	UFUNCTION()
	void HandleOwnerMovementModeChanged(ACharacter* Character, EMovementMode PrevMovementMode, uint8 PreviousCustomMode);

	UFUNCTION()
	void HandleOwnerMovementUpdated(float DeltaSeconds, FVector OldLocation, FVector OldVelocity);

	void HandleLandedMovement(const FVector& NewLandVelocity);
	void HandleJumpedMovement(float GroundSpeedBeforeJump);


protected:
	// [추가] 각 Gait의 최대 속도 (X = 앞, Y = 옆, Z = 뒤)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching|Movement|Speed")
	FVector WalkSpeeds = FVector(200.0f, 200.0f, 200.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching|Movement|Speed")
	FVector RunSpeeds = FVector(400.0f, 400.0f, 400.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching|Movement|Speed")
	FVector SprintSpeeds = FVector(630.0f, 630.0f, 630.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching|Movement|Speed")
	FVector CrouchSpeeds = FVector(170.0f, 170.0f, 170.0f);

	// [추가] 옆걸음(Strafe) 중 이동 방향 → 0 = 앞, 1 = 옆, 2 = 뒤 로 바꾸는 커브
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching|Movement|Speed")
	TObjectPtr<UCurveFloat> StrafeSpeedMapCurve;

	// [추가] 착지 직후 "방금 착지함" 상태를 유지하는 시간 (애님 착지 모션 판정용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching|Movement|Landing")
	float JustLandedDuration = 0.3f;

	// [추가] A-6: 기본 가속도 (질주가 아닐 때)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching|Movement|Acceleration")
	float DefaultMaxAcceleration = 800.0f;

	// [추가] 질주 중 수평 속도 범위 (X → Y). 이 범위를 아래 가속도 범위로 변환
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching|Movement|Acceleration")
	FVector2D SprintAccelerationSpeedRange = FVector2D(300.0f, 700.0f);

	// [추가] 질주 중 가속도 범위 (느릴 때 X, 빠를 때 Y)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching|Movement|Acceleration")
	FVector2D SprintAccelerationRange = FVector2D(800.0f, 300.0f);

	// [추가] 입력이 있을 때 / 없을 때 감속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching|Movement|Acceleration")
	float BrakingDecelerationWithInput = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching|Movement|Acceleration")
	float BrakingDecelerationWithoutInput = 2000.0f;

	// [추가] 기본 지면 마찰 (질주가 아닐 때)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching|Movement|Acceleration")
	float DefaultGroundFriction = 5.0f;

	// [추가] 질주 중 수평 속도 범위 (X → Y). 이 범위를 아래 마찰 범위로 변환
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching|Movement|Acceleration")
	FVector2D SprintFrictionSpeedRange = FVector2D(0.0f, 500.0f);

	// [추가] 질주 중 지면 마찰 범위 (느릴 때 X, 빠를 때 Y)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching|Movement|Acceleration")
	FVector2D SprintGroundFrictionRange = FVector2D(5.0f, 3.0f);

	// [추가] 입력이 없을 때 제동 마찰
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching|Movement|Acceleration")
	float BrakingFrictionWithoutInput = 3.0f;

	// [추가] A-6: 현재 이동 속도 배율 (SetMovementSpeedMultiplier로 변경)
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Replicated, Category = "MotionMatching|Movement|State")
	float MovementSpeedMultiplier = 1.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "MotionMatching|Movement|State")
	EMotionMatchingGait Gait = EMotionMatchingGait::Run;

	// [추가] 다른 클라이언트에도 보여야 하는 입력 상태 (소유 클라이언트는 직접 바꾸므로 제외)
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Replicated, Category = "MotionMatching|Movement|State")
	FMotionMatchingInputState CharacterInputState;

	// [추가] 걷기 강제 중 여부 / 강제 전 걷기 상태 (조종하는 클라이언트 전용)
	bool bIsForceWalking = false;
	bool bIsWalkingBeforeForce = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "MotionMatching|Movement|State")
	FVector LandVelocity = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "MotionMatching|Movement|State")
	bool bIsJustLanded = false;

	bool bIsWasMovingOnGroundLastFrameSimulated = true;

	FTimerHandle JustLandedTimerHandle;
#pragma endregion


#pragma region Traversal
public:
	// 트래버설이 진행 중이거나 이번 입력으로 시작에 성공하면 true (실패하면 캐릭터가 점프한다)
	UFUNCTION(BlueprintCallable, Category = "MotionMatching|Movement|Traversal") // [변경] 카테고리 정리
	bool TryTraversalFromInput();

	UFUNCTION(BlueprintPure, Category = "MotionMatching|Movement|Traversal") // [변경] 카테고리 정리
	bool IsDoingTraversalAction() const { return bIsDoingTraversalAction; }

	UFUNCTION(BlueprintPure, Category = "MotionMatching|Movement|Traversal") // [변경] 카테고리 정리
	FMotionMatchingTraversalCheckResult GetTraversalResult() const { return TraversalResult; }

	// [추가] A-6: 지금 트래버설이 가능한지 검사만 한다 (실행하지 않음). GA 활성화 조건 등에 사용
	UFUNCTION(BlueprintCallable, Category = "MotionMatching|Movement|Traversal")
	bool CanTraverse() const;

	// [추가] v0.0.7: 트래버설 중에만 ASC에 붙일 태그 등록 (캐릭터 생성자·BeginPlay 등에서 호출). 트래버설 중에 바꾸면 바로 반영
	UFUNCTION(BlueprintCallable, Category = "MotionMatching|Movement|Traversal")
	void AddTraversalOwnedTag(FGameplayTag Tag);

	// [추가] v0.0.7: 트래버설 중 태그 목록을 통째로 교체 (빈 목록 = 사용 안 함)
	UFUNCTION(BlueprintCallable, Category = "MotionMatching|Movement|Traversal")
	void SetTraversalOwnedTags(const FGameplayTagContainer& NewTags);

public:
	// [추가] A-6: 트래버설 시작/종료 알림 (각 화면에서 실행될 때마다 호출)
	UPROPERTY(BlueprintAssignable, Category = "MotionMatching|Movement|Traversal")
	FOnMotionMatchingTraversalStarted OnTraversalStarted;

	UPROPERTY(BlueprintAssignable, Category = "MotionMatching|Movement|Traversal")
	FOnMotionMatchingTraversalFinished OnTraversalFinished;

protected:
	FMotionMatchingTraversalCheckInputs GetTraversalCheckInputs() const;
	// [추가] A-6: 트레이스 + Chooser로 가능한 동작과 몽타주 후보를 구한다 (상태를 바꾸지 않음)
	bool CheckTraversal(const FMotionMatchingTraversalCheckInputs& CheckInputs, EDrawDebugTrace::Type DebugType, FMotionMatchingTraversalCheckResult& CheckResult, TArray<UObject*>& ValidMontages) const;
	bool TryTraversalAction(const FMotionMatchingTraversalCheckInputs& CheckInputs, EDrawDebugTrace::Type DebugType);
	void PerformTraversalAction();
	void UpdateWarpTargets();
	void OnTraversalStart();
	void OnTraversalEnd();
	void FinishTraversalAction();
	void ScheduleTraversalEnd(); // [추가]
	bool SetInteractionTransformOnAnimInstance(UAnimInstance* AnimInstance, const FTransform& InteractionTransform) const;

	// BP PlayMontage 노드와 동일: 중단되면 블렌드아웃 시작 시점, 정상 종료면 끝난 시점에 마무리
	void HandleTraversalMontageBlendingOut(UAnimMontage* Montage, bool bIsInterrupted);
	void HandleTraversalMontageEnded(UAnimMontage* Montage, bool bIsInterrupted);
	
	// [추가] v0.0.7: TraversalOwnedTags를 주인 액터의 ASC에 붙이거나 뗀다
	void SetTraversalOwnedTagsApplied(bool bIsApplied);
	UObject* CallAbilitySystemLibraryFunction(FName FunctionName, AActor* TargetActor) const;
	

protected:
	// 트래버설 몽타주 후보를 고르는 Chooser (CBP Details에서 CHT_TraversalAnims 지정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MotionMatching|Movement|Traversal") // [변경] 카테고리 정리
	TObjectPtr<UChooserTable> TraversalAnimsChooserTable;

	// ABP의 Pose History 노드 이름 (MotionMatch가 이 이름으로 포즈 기록을 찾는다)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MotionMatching|Movement|Traversal") // [변경] 카테고리 정리
	FName PoseHistoryName = TEXT("PoseHistory");

	// 몽타주 종료 후 서버 위치 보정을 다시 켜기까지 기다리는 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MotionMatching|Movement|Traversal") // [변경] 카테고리 정리
	float IgnoreCorrectionsDelay = 0.2f;

	// [추가] 서버: 클라이언트의 종료 알림이 끝내 오지 않을 때 위치 보정을 다시 켜기까지의 최대 대기 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MotionMatching|Movement|Traversal")
	float ClientFinishWaitTimeout = 1.0f;

	// [변경] A-6: BP에서 함수 IsDoingTraversalAction과 표시 이름이 겹치지 않도록 DisplayName 지정
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "MotionMatching|Movement|Traversal", meta = (DisplayName = "Traversal Running (State)"))
	bool bIsDoingTraversalAction = false;

	FTimerHandle IgnoreCorrectionsTimerHandle;

	// [추가] 서버: 조종 중인 클라이언트가 자기 화면의 트래버설을 끝냈는지
	bool bIsClientTraversalFinished = true;
	
	
	// [추가] v0.0.7: 트래버설(파쿠르) 중에만 주인 액터의 ASC에 붙는 태그. 비우면 사용 안 함
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching|Movement|Traversal")
	FGameplayTagContainer TraversalOwnedTags;

	bool bHasAppliedTraversalOwnedTags = false;

	// 태그를 붙인 ASC의 주인 (뗄 때 같은 ASC에서 떼기 위해)
	TWeakObjectPtr<AActor> TraversalOwnedTagsTarget;
#pragma endregion

#pragma region Sound
public:
	// [추가] 캐릭터가 점프/착지를 감지하면 호출 → 이벤트 알림 + 기본 소리 재생
	void HandleMovementJumped(float GroundSpeedBeforeJump);
	void HandleMovementLanded(const FVector& NewLandVelocity); // [변경] 멤버 LandVelocity와 이름 충돌(C4458) 방지

	// [추가] A-5: 걸음 소리 노티파이(BP_AnimNotify_FoleyEvent)가 호출. 캐릭터 BP에 인터페이스를 구현할 필요 없음
	UFUNCTION(BlueprintPure, Category = "MotionMatching|Sound|Footstep")
	UDataAsset* GetFoleyAudioBank() const { return FoleyAudioBank; }

	// [추가] 땅 위에 있거나 트래버설 중일 때만 걸음 소리 재생
	UFUNCTION(BlueprintPure, Category = "MotionMatching|Sound|Footstep")
	bool CanPlayFootstepSounds() const;

	// [추가] 액터에서 MotionMatchingComponent를 찾는다 (없으면 nullptr). 노티파이·외부 BP용
	UFUNCTION(BlueprintPure, Category = "MotionMatching", meta = (DefaultToSelf = "Actor"))
	static UMotionMatchingComponent* FindMotionMatchingComponent(const AActor* Actor);

public:
	// [추가] 점프할 때 알림 (기본 소리 대신 BP에서 직접 처리하고 싶을 때 바인딩)
	UPROPERTY(BlueprintAssignable, Category = "MotionMatching|Movement")
	FOnMotionMatchingMovementJumped OnMovementJumped;

	// [추가] 착지할 때 알림
	UPROPERTY(BlueprintAssignable, Category = "MotionMatching|Movement")
	FOnMotionMatchingMovementLanded OnMovementLanded;

protected:
	void PlaySoundAtOwnerMesh(USoundBase* Sound, float VolumeMultiplier) const;

protected:
	// [추가] 끄면 아래 점프/착지 소리를 재생하지 않음 (BP에서 OnMovementJumped/Landed로 직접 처리할 때)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching|Sound")
	bool bIsDefaultSoundEnabled = true;

	// [추가] 점프 소리
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching|Sound|Jump")
	TObjectPtr<USoundBase> JumpSound;

	// [추가] 점프 직전 수평 속도 범위 (X → Y). 이 범위를 아래 볼륨 범위로 변환
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching|Sound|Jump")
	FVector2D JumpSpeedRangeForVolume = FVector2D(0.0f, 500.0f);

	// [추가] 점프 소리 볼륨 범위 (느릴 때 X, 빠를 때 Y)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching|Sound|Jump")
	FVector2D JumpVolumeRange = FVector2D(0.5f, 1.0f);

	// [추가] 착지 소리
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching|Sound|Land")
	TObjectPtr<USoundBase> LandSound;

	// [추가] 착지 순간 낙하 속도(Z) 범위 (X → Y, 아래 방향이라 음수). 이 범위를 아래 볼륨 범위로 변환
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching|Sound|Land")
	FVector2D LandFallSpeedRangeForVolume = FVector2D(-500.0f, -900.0f);

	// [추가] 착지 소리 볼륨 범위 (약하게 떨어질 때 X, 세게 떨어질 때 Y)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching|Sound|Land")
	FVector2D LandVolumeRange = FVector2D(0.5f, 1.5f);

	// [추가] A-5: 걸음 소리 데이터 (기본: DefaultFoleyEventAudioBank). 비우면 걸음 소리가 나지 않음
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching|Sound|Footstep")
	TObjectPtr<UDataAsset> FoleyAudioBank;
#pragma endregion

#pragma region Network
protected:
	// [추가] 캐릭터에서 옮겨옴
	UFUNCTION(Server, Unreliable)
	void ServerRPC_UpdateInputState(const FMotionMatchingInputState& NewCharacterInputState);

	UFUNCTION(Server, Reliable)
	void ServerRPC_PerformTraversal(const FMotionMatchingTraversalCheckResult& NewTraversalResult);

	// [추가] 클라이언트 → 서버: 내 화면의 트래버설이 끝났다
	UFUNCTION(Server, Reliable)
	void ServerRPC_FinishTraversal();

	UFUNCTION()
	void OnRep_TraversalResult();

protected:
	// 소유 클라이언트는 이미 로컬에서 실행했으므로 제외하고 복제 (COND_SkipOwner)
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_TraversalResult, Category = "MotionMatching|Movement|Traversal") // [변경] 카테고리 정리
	FMotionMatchingTraversalCheckResult TraversalResult;
#pragma endregion

#pragma region Helper
protected:
	ACharacter* GetOwnerCharacter() const;
	UAnimInstance* GetOwnerAnimInstance() const;
	UMotionWarpingComponent* GetOwnerMotionWarpingComponent() const;
	UMotionMatchingCharacterMovementComponent* GetOwnerMotionMatchingMovement() const; // [추가] A-2
#pragma endregion
};
