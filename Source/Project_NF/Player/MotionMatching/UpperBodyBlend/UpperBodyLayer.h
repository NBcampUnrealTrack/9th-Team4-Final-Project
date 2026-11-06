#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Animation/AnimInstance.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UpperBodyLayer.generated.h"

class UAnimSequenceBase;
class UBlendSpace;
class UAnimMontage;
class UPoseSearchDatabase;
class UMeshComponent;
class ACharacter;

// MotionMatchingUpperBody 플러그인이 생성한 파일 (템플릿 v0.0.2 + Project_NF 수동 갱신: 보이는 메시 왼손 IK, 함수 이름 v0.0.3 예정분). 무기 하나의 상체 애니 (빈 칸 = 원래 모션매칭)
UCLASS(BlueprintType)
class PROJECT_NF_API UUpperBodyLayer : public UDataAsset
{
	GENERATED_BODY()

public:
	UAnimSequenceBase* GetPoseAnimation(int32 StateIndex) const;

	// ─ 1. Combat (전투 태세) ─ 사격 중·줌 중·사격 직후. 몸이 카메라 방향, 질주 막힘
	// 필수 칸: Aim Idle, Aim Offset. 칸 종류가 다른 애니(몽타주 등)나 캐릭터와 다른 스켈레톤 애니는 넣지 말 것

	// [필수] 전투 태세에서 총을 겨누고 서 있는 상체 자세 (ADS Idle 같은 정지 애니 시퀀스). 걷거나 뛸 때도 상체는 이 자세
	// 비우면: 전투 중에도 총을 겨누지 않고 평상시 칸을 씀
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "1. Combat (전투)", meta = (DisplayName = "Aim Idle (필수)", AllowedClasses = "/Script/Engine.AnimSequence"))
	TObjectPtr<UAnimSequenceBase> AimIdle;

	// [필수] 조준 방향으로 상체를 위아래·좌우로 돌리는 Aim Offset (축이 각도든 -1~1 비율이든 자동 변환)
	// 비우면: 위아래 조준이 안 됨 (좌우는 허리 비틀기가 따로 처리)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "1. Combat (전투)", meta = (DisplayName = "Aim Offset (필수)"))
	TObjectPtr<UBlendSpace> AimOffset;

	// [권장] 공격(사격) 1회마다 재생할 반동 몽타주. 몽타주 슬롯을 UpperBodyGroup.UpperBody로, 루트 모션 끔. Additive 가능. NotifyUpperBodyOnFire가 재생
	// 비우면: 사격 반동 없음
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "1. Combat (전투)", meta = (DisplayName = "Attack Montage (권장)"))
	TObjectPtr<UAnimMontage> FireMontage;

	// [선택] 전투 중 이동할 때의 다리 동작 (총 든 채 앞·뒤·옆걸음 Walk·Jog 애니 시퀀스 목록). [저장]·[적용] 때 Aim Locomotion Database를 자동 생성
	// 추천: 비움 → 전투 중에도 다리는 원래 모션매칭
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "1. Combat (전투)", meta = (DisplayName = "Aim Locomotion Anims (선택)", AllowedClasses = "/Script/Engine.AnimSequence"))
	TArray<TObjectPtr<UAnimSequenceBase>> AimLocomotion;

	// [자동] 전투 이동 다리 모션매칭 데이터베이스. Aim Locomotion Anims가 있으면 자동 생성·지정 (직접 만든 데이터베이스를 넣어도 됨)
	// 추천: 비움(None)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "1. Combat (전투)", meta = (DisplayName = "Aim Locomotion Database (자동)"))
	TObjectPtr<UPoseSearchDatabase> AimLocomotionDatabase;

	// 켜면 이 무기를 드는 동안 사격·줌과 상관없이 항상 전투 태세. 추천: 끔 (저격총처럼 늘 겨누는 무기만 켬)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "1. Combat (전투)", meta = (DisplayName = "Always Combat"))
	bool bAlwaysCombat = false;

	// 사격을 멈춘 뒤(방아쇠를 떼거나 마지막 발사 뒤) 전투 태세를 더 유지하는 시간(초). 연타 사이에 몸이 풀렸다 돌아오는 것을 막음. 0 = 바로 평상시
	// 추천: 0.2
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "1. Combat (전투)", meta = (DisplayName = "Combat Hold Time", ClampMin = "0.0"))
	float CombatHoldTime = 0.2f;

	// 허리가 전투 자세를 따르는 정도 (0~1). 낮을수록 걸을 때 몸통 흔들림이 남음. 팔·목은 항상 전투 자세. 추천: 0.5 (총이 흔들려 보이면 0.7~0.8)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "1. Combat (전투)", meta = (DisplayName = "Aim Spine Weight", ClampMin = "0.0", ClampMax = "1.0"))
	float SpineAimWeight = 0.5f;

	// 전투 태세로 들어가고 나올 때 섞는 시간(초). 추천: 0.15
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "1. Combat (전투)", meta = (DisplayName = "Aim Blend Time", ClampMin = "0.0"))
	float AimBlendTime = 0.15f;

	// [추가] 전투 태세에서 평상시로 돌아올 때만 섞는 시간(초). 진입(첫 발)은 Aim Blend Time 그대로. 0이면 Aim Blend Time 사용. 추천: 0.25
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "1. Combat (전투)", meta = (DisplayName = "Aim Blend Out Time", ClampMin = "0.0"))
	float AimBlendOutTime = 0.25f;

	// 조준 각도를 따라가는 속도 (클수록 빠름, 0 = 즉시). 추천: 15
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "1. Combat (전투)", meta = (DisplayName = "Aim Follow Speed", ClampMin = "0.0"))
	float AimInterpSpeed = 15.0f;

	// ─ 2. NonCombat (평상시) ─ 사격·줌을 안 할 때. 몸은 이동 방향
	// 모든 칸 추천: 비움(None) → 원래 모션매칭 상체. 넣는다면 총을 든 "정지 자세" 애니 시퀀스 (걷기 사이클을 넣으면 상체만 따로 흔들림)

	// 서 있을 때 상체 자세. 추천: 비움(None)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "2. NonCombat (평상시)", meta = (DisplayName = "NonCombat Idle", AllowedClasses = "/Script/Engine.AnimSequence"))
	TObjectPtr<UAnimSequenceBase> Idle;

	// 걸을 때 상체 자세. 추천: 비움(None)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "2. NonCombat (평상시)", meta = (DisplayName = "NonCombat Walk", AllowedClasses = "/Script/Engine.AnimSequence"))
	TObjectPtr<UAnimSequenceBase> Walk;

	// 뛸 때 상체 자세. 추천: 비움(None)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "2. NonCombat (평상시)", meta = (DisplayName = "NonCombat Run", AllowedClasses = "/Script/Engine.AnimSequence"))
	TObjectPtr<UAnimSequenceBase> Run;

	// 질주할 때 상체 자세 (총을 내린 자세 권장). 추천: 비움(None)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "2. NonCombat (평상시)", meta = (DisplayName = "NonCombat Sprint", AllowedClasses = "/Script/Engine.AnimSequence"))
	TObjectPtr<UAnimSequenceBase> Sprint;

	// 공중(점프·낙하)일 때 상체 자세. 추천: 비움(None)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "2. NonCombat (평상시)", meta = (DisplayName = "NonCombat Jump/Fall", AllowedClasses = "/Script/Engine.AnimSequence"))
	TObjectPtr<UAnimSequenceBase> Air;

	// 앉았을 때 상체 자세. 추천: 비움(None)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "2. NonCombat (평상시)", meta = (DisplayName = "NonCombat Crouch", AllowedClasses = "/Script/Engine.AnimSequence"))
	TObjectPtr<UAnimSequenceBase> Crouch;

	// 평상시 자세가 바뀔 때 섞는 시간(초). 추천: 0.2
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "2. NonCombat (평상시)", meta = (DisplayName = "NonCombat Blend Time", ClampMin = "0.0"))
	float BlendTime = 0.2f;

	// ─ 3. Left Hand IK (두 손 무기) ─

	// 두 손 무기면 켜기 → 왼손을 무기 메시 소켓에 붙임 (Set Upper Body Two Hand IK Weapon으로 무기 메시를 지정해야 동작). 추천: 권총은 끔
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "3. Left Hand IK (두 손 무기)", meta = (DisplayName = "Two Handed"))
	bool bIsTwoHanded = false;

	// 무기 메시에서 왼손을 붙일 소켓 이름. 추천: LeftHandIK (무기 메시에 이 이름의 소켓을 만들기)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "3. Left Hand IK (두 손 무기)", meta = (DisplayName = "Left Hand Socket", EditCondition = "bIsTwoHanded"))
	FName LeftHandSocketName = TEXT("LeftHandIK");
};


// 상체 레이어 ABP의 부모. 캐릭터의 무기 Enum 값 → 데이터 에셋 → 이동 상태별 칸을 골라 그래프에 넘김
UCLASS()
class PROJECT_NF_API UUpperBodyLayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UUpperBodyLayerAnimInstance();

	void SetWeaponMesh(UMeshComponent* NewWeaponMesh);
	void SetZoomHeld(bool bIsHeld);
	void SetFireHeld(bool bIsHeld);

	// 사격 알림: 전투 태세로 전환·유지하고 FireMontage 재생. 반환값 = 지금 총구가 조준 방향을 향하고 있음
	bool NotifyFire();

	// 지금 조준 자세가 다 잡혔는지 (읽기 전용, NotifyFire 반환과 같은 기준). 무기 데이터 에셋이 없으면(상체 레이어 꺼짐) 기다릴 자세가 없으므로 true
	bool IsAimPoseReady() const;

	// [v0.0.3 예정분] 보이는 메시 왼손 IK(UUpperBodyLayerHandIKAnimInstance)가 읽는 값. 총이 어느 메시에 붙었는지와 상관없이 계산한 값
	UMeshComponent* GetWeaponMesh() const { return WeaponMesh.Get(); }
	FName GetLeftHandSocketName() const { return LeftHandSocketName; }
	float GetLeftHandIKTargetAlpha() const { return LeftHandIKTargetAlpha; }

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeUninitializeAnimation() override;

	// 캐릭터의 무기 Enum 변수 이름 (플러그인 창이 채움)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UpperBody|Setup")
	FName WeaponEnumPropertyName;

	// 구동용 ABP의 Gait 변수 이름 (플러그인 창이 채움)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UpperBody|Setup")
	FName GaitPropertyName;

	// 구동용 ABP의 조준 여부 bool 경로 (예: OwnerInputState.bIsWantsToAim, 플러그인 창이 채움, 비우면 조준 없음)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UpperBody|Setup")
	FString AimStatePropertyPath;

	// 평상시 태세 고개 돌리기 Aim Offset (구동용 ABP의 Yaw·Pitch 블렌드 스페이스, 플러그인 창이 채움. 예: BS_Neutral_AO_Stand)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UpperBody|Setup")
	TObjectPtr<UBlendSpace> RelaxedAimOffset;

	// 캐릭터 컴포넌트의 조준 요청 함수 이름 (예: SetWantsToAim, 플러그인 창이 채움)
	// "항상 조준" 무기를 들면 이 함수로 조준을 켜서 몸이 카메라 방향을 보게 함 (질주·무기 해제 때 끔)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UpperBody|Setup")
	FName AimRequestFunctionName;

	// 캐릭터 컴포넌트의 기능 켜기/끄기 함수 이름 (예: SetFeatureEnabled(Enum, bool), Enum에 Sprint 항목, 플러그인 창이 채움)
	// 있으면 전투 태세가 질주보다 우선: 전투 태세 동안 질주 기능을 꺼서 질주 취소 + 질주 입력 무시, 끝나면 다시 켬
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UpperBody|Setup")
	FName SprintBlockFunctionName;

	// Enum 항목 이름 → 데이터 에셋 (없는 항목 = 원래 모션매칭)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UpperBody|Setup")
	TMap<FName, TObjectPtr<UUpperBodyLayer>> DataAssetByEntryName;

	// 이 속도 미만이면 서 있음 칸
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UpperBody|Setup")
	float IdleSpeedThreshold = 10.0f;

	// 조준 좌우 각도 제한 (몸 정면 기준, 넘으면 이 값에서 멈춤)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UpperBody|Setup")
	float MaxAimYaw = 90.0f;

	// 전투 태세에서 몸이 조준 방향으로 돌아오는 동안 허리를 비트는 최대 각도 (다리는 제자리 회전으로 따라옴)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UpperBody|Setup")
	float MaxTurnTwist = 135.0f;

	UPROPERTY(BlueprintReadOnly, Category = "UpperBody|Runtime")
	TObjectPtr<UAnimSequenceBase> PoseAnimationA;

	UPROPERTY(BlueprintReadOnly, Category = "UpperBody|Runtime")
	TObjectPtr<UAnimSequenceBase> PoseAnimationB;

	// 0 = A, 1 = B
	UPROPERTY(BlueprintReadOnly, Category = "UpperBody|Runtime")
	float PoseBlendAlpha = 0.0f;

	// 0 = 원래 모션매칭, 1 = 상체 애니
	UPROPERTY(BlueprintReadOnly, Category = "UpperBody|Runtime")
	float UpperBodyWeight = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "UpperBody|Runtime")
	FVector LeftHandIKLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "UpperBody|Runtime")
	float LeftHandIKAlpha = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "UpperBody|Runtime")
	TObjectPtr<UBlendSpace> CurrentAimOffset;

	// Aim Offset 입력값 (Aim Offset 축 단위로 변환된 값)
	UPROPERTY(BlueprintReadOnly, Category = "UpperBody|Runtime")
	float AimYaw = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "UpperBody|Runtime")
	float AimPitch = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "UpperBody|Runtime")
	float AimOffsetAlpha = 0.0f;

	// 조준 각도(도, 보간 후). 조준 방향 - 캐릭터 방향
	UPROPERTY(BlueprintReadOnly, Category = "UpperBody|Runtime")
	float AimYawDegrees = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "UpperBody|Runtime")
	float AimPitchDegrees = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "UpperBody|Runtime")
	bool bIsAiming = false;

	// 0 = 평소, 1 = 조준
	UPROPERTY(BlueprintReadOnly, Category = "UpperBody|Runtime")
	float AimAlpha = 0.0f;

	// 평상시 태세 고개 돌리기 (모션매칭 Strafe 때 쓰는 고개 Aim Offset과 같은 에셋)
	UPROPERTY(BlueprintReadOnly, Category = "UpperBody|Runtime")
	float RelaxedAimYaw = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "UpperBody|Runtime")
	float RelaxedAimPitch = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "UpperBody|Runtime")
	float RelaxedAimAlpha = 0.0f;

	// 허리 단계 블렌드 비율 (데이터 에셋 SpineAimWeight)
	UPROPERTY(BlueprintReadOnly, Category = "UpperBody|Runtime")
	float SpineBlendWeight = 1.0f;

	// 조준 이동 모션매칭 노드가 검색할 데이터베이스
	UPROPERTY(BlueprintReadOnly, Category = "UpperBody|Runtime")
	TObjectPtr<UPoseSearchDatabase> CurrentAimDatabase;

	// 0 = 원래 다리, 1 = 조준 이동 모션매칭 (전신)
	UPROPERTY(BlueprintReadOnly, Category = "UpperBody|Runtime")
	float AimLocomotionAlpha = 0.0f;

	// 허리 비틀기 (몸 방향 → 캡슐 방향 남은 각도). 그래프의 허리 본 3개에 나눠 더함
	UPROPERTY(BlueprintReadOnly, Category = "UpperBody|Runtime")
	float TurnTwistYawDegrees = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "UpperBody|Runtime")
	FRotator TurnTwistRotation = FRotator::ZeroRotator;

private:
	UUpperBodyLayer* FindCurrentDataAsset(const ACharacter* Character) const;
	int32 GetCurrentStateIndex(const ACharacter* Character) const;
	bool ReadIsAiming() const;
	bool IsFullBodyActionPlaying() const;
	bool UpdateCombatState(ACharacter* Character, const UUpperBodyLayer* DataAsset, int32 StateIndex, float DeltaSeconds);
	void UpdateRelaxedAimOffset(const UUpperBodyLayer* DataAsset, int32 StateIndex, float DeltaSeconds);
	bool CallAimRequest(ACharacter* Character, bool bIsAimRequested) const;
	void UpdateSprintBlock(ACharacter* Character, bool bIsCombat);
	void ReleaseLocalRequests(ACharacter* Character);
	bool CallSprintBlock(ACharacter* Character, bool bIsSprintEnabled) const;
	bool IsSprintFeatureEnabled(const ACharacter* Character) const;
	void UpdateTurnTwist(const ACharacter* Character);
	void UpdatePoseAnimation(UAnimSequenceBase* TargetAnimation);
	void UpdateLeftHandIK(const UUpperBodyLayer* DataAsset, float DeltaSeconds);
	void UpdateAimLocomotion(const UUpperBodyLayer* DataAsset, int32 StateIndex, float DeltaSeconds, float AimBlendSpeed);
	void UpdateAimOffset(const ACharacter* Character, const UUpperBodyLayer* DataAsset, float DeltaSeconds);

	static FName ReadEnumEntryName(const UObject* Owner, FName PropertyName);
	static bool ReadBoolByPath(const UObject* Owner, const FString& PropertyPath);
	static float ConvertToAimOffsetAxis(const UBlendSpace* AimOffsetAsset, int32 AxisIndex, float Degrees);

	TWeakObjectPtr<UMeshComponent> WeaponMesh;
	bool bIsPoseBActive = false;
	bool bHasRequestedAim = false;
	bool bHasBlockedSprint = false;
	float AimResidualYawDegrees = 0.0f;
	bool bIsZoomHeld = false;
	bool bIsFireHeld = false;
	float CombatTimeRemaining = 0.0f;

	// [추가] 조준을 시작한 순간의 DA. 조준 중 DA가 바뀌면(근접공격 종료 직후 무기 이넘 복원 등) 조준이 끝날 때까지 새 DA의 AimIdle을 쓰지 않음
	TWeakObjectPtr<const UUpperBodyLayer> AimStartDataAsset;

	FName LeftHandSocketName = NAME_None;
	float LeftHandIKTargetAlpha = 0.0f;

	// 전신 동작(파쿠르 등) 전후로 왼손 IK를 서서히 끄고 켜는 배율 (0 = 해제, 1 = 적용)
	float FullBodyActionIKScale = 1.0f;
};


// [v0.0.3 예정분] 보이는 메시(VisualMesh 등) ABP의 부모. 총이 이 메시에 붙어 있으면 상체 레이어가 계산한 왼손 IK를 이 메시에 적용
// 목표 위치 = 오른손(hand_r) 뼈 기준 상대 위치 → 이동·회전 중에도 왼손이 밀리지 않음
// 그래프: (기존 포즈) → Local To Component → Two Bone IK(hand_l, Effector = Bone Space / hand_r, LeftHandIKOffset, LeftHandIKAlpha) → Component To Local
UCLASS()
class PROJECT_NF_API UUpperBodyLayerHandIKAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// 오른손 뼈 공간 기준 왼손 목표 위치
	UPROPERTY(BlueprintReadOnly, Category = "UpperBody|Runtime")
	FVector LeftHandIKOffset = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "UpperBody|Runtime")
	float LeftHandIKAlpha = 0.0f;
};


UCLASS()
class PROJECT_NF_API UUpperBodyLayerLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 두 손 무기 왼손 IK가 따라갈 무기 메시 지정 (무기를 바꿀 때 호출, 해제는 None). 모든 화면에서 호출
	UFUNCTION(BlueprintCallable, Category = "UpperBody")
	static void SetUpperBodyTwoHandIKWeapon(ACharacter* Character, UMeshComponent* WeaponMesh);

	// 1발마다 호출 (조종하는 쪽): 전투 태세로 전환·유지 + 무기 FireMontage 재생. 반환값 = 총구가 이미 조준 방향을 향함
	UFUNCTION(BlueprintCallable, Category = "UpperBody")
	static bool NotifyUpperBodyOnFire(ACharacter* Character);

	// 줌 시작/끝 (조종하는 쪽): 줌 중 전투 태세 유지
	UFUNCTION(BlueprintCallable, Category = "UpperBody")
	static void SetUpperBodyCombatStanceByZoom(ACharacter* Character, bool bIsZooming);

	// 사격 키(방아쇠) 누름/뗌 (조종하는 쪽): 누르고 있는 동안 전투 태세. 떼면 CombatHoldTime 뒤 평상시
	UFUNCTION(BlueprintCallable, Category = "UpperBody")
	static void SetUpperBodyCombatStanceByFire(ACharacter* Character, bool bIsFiring);

	// 조준 자세 준비 여부 (총알을 쏘기 직전에 확인, 서버에서도 읽음). 상체 레이어가 없거나 데이터 에셋이 없는 무기면 true
	UFUNCTION(BlueprintPure, Category = "UpperBody")
	static bool IsUpperBodyAimReady(ACharacter* Character);
};
