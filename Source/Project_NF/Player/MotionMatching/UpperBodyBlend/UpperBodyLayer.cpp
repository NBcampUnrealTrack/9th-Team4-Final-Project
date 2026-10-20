#include "UpperBodyLayer.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/BlendSpace.h"
#include "Animation/AnimMontage.h"
#include "PoseSearch/PoseSearchDatabase.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace UpperBodyLayerState
{
	const int32 Idle = 0;
	const int32 Walk = 1;
	const int32 Run = 2;
	const int32 Sprint = 3;
	const int32 Air = 4;
	const int32 Crouch = 5;
}

namespace UpperBodyLayerTurnTwist
{
	// 비틀기를 나눠 받는 허리 본 수 (상체 레이어 그래프 spine_01·02·03)
	const float BoneCount = 3.0f;
}


UAnimSequenceBase* UUpperBodyLayer::GetPoseAnimation(int32 StateIndex) const
{
	if (StateIndex == UpperBodyLayerState::Idle) return Idle;
	if (StateIndex == UpperBodyLayerState::Walk) return Walk;
	if (StateIndex == UpperBodyLayerState::Run) return Run;
	if (StateIndex == UpperBodyLayerState::Sprint) return Sprint;
	if (StateIndex == UpperBodyLayerState::Air) return Air;
	if (StateIndex == UpperBodyLayerState::Crouch) return Crouch;

	return nullptr;
}


UUpperBodyLayerAnimInstance::UUpperBodyLayerAnimInstance()
{
	// 구동용 ABP에서 재생한 몽타주가 이 ABP의 UpperBody 슬롯에서도 재생되도록
	SetUseMainInstanceMontageEvaluationData(true);
}


void UUpperBodyLayerAnimInstance::SetWeaponMesh(UMeshComponent* NewWeaponMesh)
{
	WeaponMesh = NewWeaponMesh;
}


void UUpperBodyLayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	auto* Character = Cast<ACharacter>(TryGetPawnOwner());
	if (IsValid(Character) == false) return;

	const UUpperBodyLayer* DataAsset = FindCurrentDataAsset(Character);
	const int32 StateIndex = GetCurrentStateIndex(Character);
	const bool bIsLocalCombat = UpdateCombatState(Character, DataAsset, StateIndex, DeltaSeconds);

	// 조준 자세 여부 = 컴포넌트의 조준 상태(복제됨 → 다른 클라이언트도 같은 값)
	// 질주 막기 함수가 없으면 질주 중에는 무시 (질주 중 사격 불가 규칙), 있으면 전투가 질주를 끄므로 바로 조준
	// 조준 상태를 못 읽는 프로젝트면 이 화면의 전투 태세 판정을 그대로 사용
	bIsAiming = false;
	if (IsValid(DataAsset) == true && IsFullBodyActionPlaying() == false && (SprintBlockFunctionName.IsNone() == false || StateIndex != UpperBodyLayerState::Sprint))
	{
		if (AimStatePropertyPath.IsEmpty() == true)
		{
			bIsAiming = bIsLocalCombat;
		}
		else
		{
			bIsAiming = ReadIsAiming();
		}
	}

	float AimBlendSpeed = 1000.0f;
	if (IsValid(DataAsset) == true && DataAsset->AimBlendTime > KINDA_SMALL_NUMBER)
	{
		AimBlendSpeed = 1.0f / DataAsset->AimBlendTime;
	}

	AimAlpha = FMath::FInterpConstantTo(AimAlpha, bIsAiming == true ? 1.0f : 0.0f, DeltaSeconds, AimBlendSpeed);

	UAnimSequenceBase* TargetAnimation = nullptr;
	float BlendTime = 0.2f;
	if (IsValid(DataAsset) == true)
	{
		TargetAnimation = DataAsset->GetPoseAnimation(StateIndex);
		BlendTime = DataAsset->BlendTime;
		if (bIsAiming == true && IsValid(DataAsset->AimIdle) == true)
		{
			TargetAnimation = DataAsset->AimIdle;
			BlendTime = DataAsset->AimBlendTime;
		}
	}

	float BlendSpeed = 1000.0f;
	if (BlendTime > KINDA_SMALL_NUMBER)
	{
		BlendSpeed = 1.0f / BlendTime;
	}

	float TargetWeight = 0.0f;
	if (IsValid(TargetAnimation) == true)
	{
		UpdatePoseAnimation(TargetAnimation);
		TargetWeight = 1.0f;
	}

	UpperBodyWeight = FMath::FInterpConstantTo(UpperBodyWeight, TargetWeight, DeltaSeconds, BlendSpeed);

	float TargetPoseBlendAlpha = 0.0f;
	if (bIsPoseBActive == true)
	{
		TargetPoseBlendAlpha = 1.0f;
	}

	PoseBlendAlpha = FMath::FInterpConstantTo(PoseBlendAlpha, TargetPoseBlendAlpha, DeltaSeconds, BlendSpeed);

	SpineBlendWeight = 1.0f;
	if (IsValid(DataAsset) == true)
	{
		SpineBlendWeight = DataAsset->SpineAimWeight;
	}

	UpdateAimLocomotion(DataAsset, StateIndex, DeltaSeconds, AimBlendSpeed);
	UpdateRelaxedAimOffset(DataAsset, StateIndex, DeltaSeconds);
	UpdateTurnTwist(Character);
	UpdateAimOffset(Character, DataAsset, DeltaSeconds);
	UpdateLeftHandIK(DataAsset);
}


UUpperBodyLayer* UUpperBodyLayerAnimInstance::FindCurrentDataAsset(const ACharacter* Character) const
{
	const FName EntryName = ReadEnumEntryName(Character, WeaponEnumPropertyName);
	if (EntryName.IsNone() == true) return nullptr;

	return DataAssetByEntryName.FindRef(EntryName);
}


// 공중 → 앉기 → 서 있음(속도) → Gait(Walk·Run·Sprint) 순서로 판정
int32 UUpperBodyLayerAnimInstance::GetCurrentStateIndex(const ACharacter* Character) const
{
	const UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement();
	if (IsValid(MovementComponent) == true)
	{
		if (MovementComponent->IsFalling() == true) return UpperBodyLayerState::Air;
		if (MovementComponent->IsCrouching() == true) return UpperBodyLayerState::Crouch;
	}

	if (Character->GetVelocity().Size2D() < IdleSpeedThreshold) return UpperBodyLayerState::Idle;

	const USkeletalMeshComponent* MeshComponent = GetSkelMeshComponent();
	if (IsValid(MeshComponent) == false) return UpperBodyLayerState::Walk;

	const FName GaitName = ReadEnumEntryName(MeshComponent->GetAnimInstance(), GaitPropertyName);
	if (GaitName == FName(TEXT("Sprint"))) return UpperBodyLayerState::Sprint;
	if (GaitName == FName(TEXT("Run"))) return UpperBodyLayerState::Run;

	return UpperBodyLayerState::Walk;
}


// 전투 태세 판정: 무기 있음 + 전신 동작(파쿠르 등) 아님 + (항상 전투 / 줌 중 / 사격 중 / 사격을 멈춘 뒤 CombatHoldTime 이내). 질주 막기 함수가 없으면 질주 중에는 전투 태세 아님
// 직접 조종하는 캐릭터는 바뀔 때만 질주 막기·조준 요청을 켜고 끔 → 질주 취소, 이동 컴포넌트가 몸을 카메라 방향으로 돌리고, 조준 상태를 서버·다른 클라이언트에 전달
bool UUpperBodyLayerAnimInstance::UpdateCombatState(ACharacter* Character, const UUpperBodyLayer* DataAsset, int32 StateIndex, float DeltaSeconds)
{
	CombatTimeRemaining = FMath::Max(0.0f, CombatTimeRemaining - DeltaSeconds);

	bool bShouldCombat = false;
	if (IsValid(DataAsset) == true && IsFullBodyActionPlaying() == false && (SprintBlockFunctionName.IsNone() == false || StateIndex != UpperBodyLayerState::Sprint))
	{
		bShouldCombat = DataAsset->bAlwaysCombat == true || bIsZoomHeld == true || bIsFireHeld == true || CombatTimeRemaining > 0.0f;
	}

	// 조종이 풀린 캐릭터(사망·빙의 해제 등)는 이 화면이 켜 둔 질주 막기·조준 요청을 되돌림
	if (Character->IsLocallyControlled() == false)
	{
		ReleaseLocalRequests(Character);
		return bShouldCombat;
	}

	UpdateSprintBlock(Character, bShouldCombat);

	if (AimRequestFunctionName.IsNone() == true) return bShouldCombat;
	if (bShouldCombat == bHasRequestedAim) return bShouldCombat;

	if (CallAimRequest(Character, bShouldCombat) == true)
	{
		bHasRequestedAim = bShouldCombat;
	}

	return bShouldCombat;
}


void UUpperBodyLayerAnimInstance::SetZoomHeld(bool bIsHeld)
{
	bIsZoomHeld = bIsHeld;
}


// 방아쇠를 떼면 CombatHoldTime만큼 더 유지 후 평상시
void UUpperBodyLayerAnimInstance::SetFireHeld(bool bIsHeld)
{
	if (bIsFireHeld == true && bIsHeld == false)
	{
		const UUpperBodyLayer* DataAsset = FindCurrentDataAsset(Cast<ACharacter>(TryGetPawnOwner()));
		if (IsValid(DataAsset) == true)
		{
			CombatTimeRemaining = FMath::Max(CombatTimeRemaining, DataAsset->CombatHoldTime);
		}
	}

	bIsFireHeld = bIsHeld;
}


// 사격: 전투 태세 시간을 CombatHoldTime으로 채움 → 다음 업데이트에서 조준 요청. 몽타주는 구동용 ABP에서 재생 (UpperBody 슬롯이 받음)
// 전투 태세(조준 상태)는 조준 요청으로 다른 클라이언트에 전달되고, 사격 몽타주를 다른 화면에도 보이려면 사격 연출 쪽(보통 멀티캐스트)에서도 호출
bool UUpperBodyLayerAnimInstance::NotifyFire()
{
	auto* Character = Cast<ACharacter>(TryGetPawnOwner());
	if (IsValid(Character) == false) return false;

	const UUpperBodyLayer* DataAsset = FindCurrentDataAsset(Character);
	if (IsValid(DataAsset) == false) return false;

	// 전신 동작 중에는 사격 무시 (같은 그룹 몽타주를 재생하면 파쿠르 몽타주가 끊김). 실제 공격을 막는 것은 프로젝트 쪽(GA 태그 등)
	if (IsFullBodyActionPlaying() == true) return false;

	CombatTimeRemaining = DataAsset->CombatHoldTime;

	const USkeletalMeshComponent* MeshComponent = GetSkelMeshComponent();
	if (IsValid(DataAsset->FireMontage) == true && IsValid(MeshComponent) == true && IsValid(MeshComponent->GetAnimInstance()) == true)
	{
		MeshComponent->GetAnimInstance()->Montage_Play(DataAsset->FireMontage);
	}

	return AimAlpha >= 0.99f && FMath::Abs(AimResidualYawDegrees) <= 15.0f;
}


// 캐릭터 컴포넌트 중 AimRequestFunctionName(bool 1개) 함수가 있는 첫 컴포넌트에 호출
bool UUpperBodyLayerAnimInstance::CallAimRequest(ACharacter* Character, bool bIsAimRequested) const
{
	TInlineComponentArray<UActorComponent*> Components(Character);
	for (UActorComponent* Component : Components)
	{
		if (IsValid(Component) == false) continue;

		UFunction* Function = Component->FindFunction(AimRequestFunctionName);
		if (Function == nullptr || Function->NumParms != 1 || Function->GetReturnProperty() != nullptr) continue;

		const FBoolProperty* Parameter = CastField<FBoolProperty>(Function->PropertyLink);
		if (Parameter == nullptr) continue;

		TArray<uint8> ParameterBuffer;
		ParameterBuffer.SetNumZeroed(Function->ParmsSize);
		Parameter->SetPropertyValue_InContainer(ParameterBuffer.GetData(), bIsAimRequested);
		Component->ProcessEvent(Function, ParameterBuffer.GetData());
		return true;
	}

	return false;
}


// 전투 태세 동안 질주 기능을 끔 (질주 취소 + 질주 입력 무시). 끝나면 이 클래스가 끈 경우만 다시 켬
// 전투 시작 때 이미 꺼져 있으면(게임 규칙으로 끈 경우) 건드리지 않음
void UUpperBodyLayerAnimInstance::UpdateSprintBlock(ACharacter* Character, bool bIsCombat)
{
	if (SprintBlockFunctionName.IsNone() == true) return;

	if (bIsCombat == true && bHasBlockedSprint == false)
	{
		if (IsSprintFeatureEnabled(Character) == false) return;

		if (CallSprintBlock(Character, false) == true)
		{
			bHasBlockedSprint = true;
		}

		return;
	}

	// 아직 꺼져 있을 때만 다시 켬 (전투 중 다른 곳에서 이미 켰으면 그대로)
	if (bIsCombat == false && bHasBlockedSprint == true)
	{
		if (IsSprintFeatureEnabled(Character) == false)
		{
			CallSprintBlock(Character, true);
		}

		bHasBlockedSprint = false;
	}
}


// 이 화면이 켜 둔 질주 막기·조준 요청을 되돌림
void UUpperBodyLayerAnimInstance::ReleaseLocalRequests(ACharacter* Character)
{
	if (bHasBlockedSprint == true)
	{
		CallSprintBlock(Character, true);
		bHasBlockedSprint = false;
	}

	if (bHasRequestedAim == true)
	{
		CallAimRequest(Character, false);
		bHasRequestedAim = false;
	}
}


// 레이어가 내려가거나 다시 연결될 때도 질주·조준 요청이 남지 않게 함
void UUpperBodyLayerAnimInstance::NativeUninitializeAnimation()
{
	auto* Character = Cast<ACharacter>(TryGetPawnOwner());
	if (IsValid(Character) == true)
	{
		ReleaseLocalRequests(Character);
	}

	Super::NativeUninitializeAnimation();
}


// 캐릭터 컴포넌트 중 SprintBlockFunctionName(Enum, bool) 함수가 있는 첫 컴포넌트에 (Sprint 항목, 켜기/끄기)로 호출
bool UUpperBodyLayerAnimInstance::CallSprintBlock(ACharacter* Character, bool bIsSprintEnabled) const
{
	TInlineComponentArray<UActorComponent*> Components(Character);
	for (UActorComponent* Component : Components)
	{
		if (IsValid(Component) == false) continue;

		UFunction* Function = Component->FindFunction(SprintBlockFunctionName);
		if (Function == nullptr || Function->NumParms != 2 || Function->GetReturnProperty() != nullptr) continue;

		TArray<FProperty*> Parameters;
		for (TFieldIterator<FProperty> ParameterIterator(Function); ParameterIterator && (ParameterIterator->PropertyFlags & CPF_Parm) != 0; ++ParameterIterator)
		{
			Parameters.Add(*ParameterIterator);
		}

		if (Parameters.Num() != 2) continue;

		const FBoolProperty* BoolParameter = CastField<FBoolProperty>(Parameters[1]);
		if (BoolParameter == nullptr) continue;

		const UEnum* Enum = nullptr;
		const FEnumProperty* EnumParameter = CastField<FEnumProperty>(Parameters[0]);
		const FByteProperty* ByteParameter = CastField<FByteProperty>(Parameters[0]);
		if (EnumParameter != nullptr) Enum = EnumParameter->GetEnum();
		if (ByteParameter != nullptr) Enum = ByteParameter->Enum;
		if (Enum == nullptr) continue;

		int32 SprintIndex = INDEX_NONE;
		for (int32 EnumIndex = 0; EnumIndex < Enum->NumEnums(); ++EnumIndex)
		{
			if (Enum->GetNameStringByIndex(EnumIndex).Equals(TEXT("Sprint")) == true)
			{
				SprintIndex = EnumIndex;
				break;
			}
		}

		if (SprintIndex == INDEX_NONE) continue;

		TArray<uint8> ParameterBuffer;
		ParameterBuffer.SetNumZeroed(Function->ParmsSize);
		if (EnumParameter != nullptr)
		{
			EnumParameter->GetUnderlyingProperty()->SetIntPropertyValue(EnumParameter->ContainerPtrToValuePtr<void>(ParameterBuffer.GetData()), Enum->GetValueByIndex(SprintIndex));
		}
		else
		{
			ByteParameter->SetPropertyValue_InContainer(ParameterBuffer.GetData(), static_cast<uint8>(Enum->GetValueByIndex(SprintIndex)));
		}

		BoolParameter->SetPropertyValue_InContainer(ParameterBuffer.GetData(), bIsSprintEnabled);
		Component->ProcessEvent(Function, ParameterBuffer.GetData());
		return true;
	}

	return false;
}


// 질주 기능이 켜져 있는지 (컴포넌트 bool 변수 중 이름에 Sprint와 Enabled가 모두 있는 것, 못 찾으면 켜져 있다고 봄)
bool UUpperBodyLayerAnimInstance::IsSprintFeatureEnabled(const ACharacter* Character) const
{
	TInlineComponentArray<UActorComponent*> Components(Character);
	for (const UActorComponent* Component : Components)
	{
		if (IsValid(Component) == false) continue;
		if (Component->FindFunction(SprintBlockFunctionName) == nullptr) continue;

		for (TFieldIterator<FBoolProperty> PropertyIterator(Component->GetClass()); PropertyIterator; ++PropertyIterator)
		{
			const FString PropertyName = PropertyIterator->GetName();
			if (PropertyName.Contains(TEXT("Sprint")) == false || PropertyName.Contains(TEXT("Enabled")) == false) continue;

			return PropertyIterator->GetPropertyValue_InContainer(Component);
		}

		return true;
	}

	return true;
}


// 구동용 ABP가 루트 모션 몽타주(파쿠르·구르기 등 전신 동작)를 재생 중인지
bool UUpperBodyLayerAnimInstance::IsFullBodyActionPlaying() const
{
	const USkeletalMeshComponent* MeshComponent = GetSkelMeshComponent();
	if (IsValid(MeshComponent) == false) return false;

	const UAnimInstance* MainInstance = MeshComponent->GetAnimInstance();
	if (IsValid(MainInstance) == false) return false;

	return MainInstance->GetRootMotionMontageInstance() != nullptr;
}


// 구동용 ABP의 조준 여부 (경로가 비어 있거나 못 찾으면 조준 안 함)
bool UUpperBodyLayerAnimInstance::ReadIsAiming() const
{
	if (AimStatePropertyPath.IsEmpty() == true) return false;

	const USkeletalMeshComponent* MeshComponent = GetSkelMeshComponent();
	if (IsValid(MeshComponent) == false) return false;

	return ReadBoolByPath(MeshComponent->GetAnimInstance(), AimStatePropertyPath);
}


// 재생 중인 칸과 다르면 비어 있는 쪽(A/B)에 새 애니를 넣고 그쪽으로 섞음
void UUpperBodyLayerAnimInstance::UpdatePoseAnimation(UAnimSequenceBase* TargetAnimation)
{
	UAnimSequenceBase* ActiveAnimation = PoseAnimationA;
	UAnimSequenceBase* InactiveAnimation = PoseAnimationB;
	if (bIsPoseBActive == true)
	{
		ActiveAnimation = PoseAnimationB;
		InactiveAnimation = PoseAnimationA;
	}

	if (ActiveAnimation == TargetAnimation) return;

	if (UpperBodyWeight <= KINDA_SMALL_NUMBER || IsValid(ActiveAnimation) == false)
	{
		PoseAnimationA = TargetAnimation;
		PoseAnimationB = TargetAnimation;
		return;
	}

	if (InactiveAnimation != TargetAnimation)
	{
		if (bIsPoseBActive == true)
		{
			PoseAnimationA = TargetAnimation;
		}
		else
		{
			PoseAnimationB = TargetAnimation;
		}
	}

	if (bIsPoseBActive == true)
	{
		bIsPoseBActive = false;
		return;
	}

	bIsPoseBActive = true;
}


// 두 손 무기 + 무기 메시 지정 + 소켓 있음일 때만 왼손을 소켓 위치로 (커브 DisableLeftHandIK = 1이면 해제)
void UUpperBodyLayerAnimInstance::UpdateLeftHandIK(const UUpperBodyLayer* DataAsset)
{
	LeftHandIKAlpha = 0.0f;
	if (IsValid(DataAsset) == false) return;
	if (DataAsset->bIsTwoHanded == false) return;

	UMeshComponent* CurrentWeaponMesh = WeaponMesh.Get();
	if (IsValid(CurrentWeaponMesh) == false) return;
	if (CurrentWeaponMesh->DoesSocketExist(DataAsset->LeftHandSocketName) == false) return;

	LeftHandIKLocation = CurrentWeaponMesh->GetSocketLocation(DataAsset->LeftHandSocketName);

	float DisableCurveValue = 0.0f;
	const USkeletalMeshComponent* MeshComponent = GetSkelMeshComponent();
	if (IsValid(MeshComponent) == true && IsValid(MeshComponent->GetAnimInstance()) == true)
	{
		DisableCurveValue = FMath::Clamp(MeshComponent->GetAnimInstance()->GetCurveValue(TEXT("DisableLeftHandIK")), 0.0f, 1.0f);
	}

	LeftHandIKAlpha = UpperBodyWeight * (1.0f - DisableCurveValue);
}


// 전투 태세에서 캡슐은 조준 방향으로 바로 돌지만 몸(루트 본)은 제자리 회전으로 늦게 따라옴
// 남은 각도(캡슐 - 몸)만큼 허리를 비틀어 상체·총이 바로 조준 방향을 보게 함. 다리가 따라온 만큼 비틀기가 줄어듦
void UUpperBodyLayerAnimInstance::UpdateTurnTwist(const ACharacter* Character)
{
	TurnTwistYawDegrees = 0.0f;
	AimResidualYawDegrees = 0.0f;

	const USkeletalMeshComponent* MeshComponent = GetSkelMeshComponent();
	if (IsValid(MeshComponent) == true && MeshComponent->GetNumBones() > 0)
	{
		const float BodyYaw = MeshComponent->GetSocketRotation(MeshComponent->GetBoneName(0)).Yaw - MeshComponent->GetRelativeRotation().Yaw;
		const float RawLagYaw = FRotator::NormalizeAxis(Character->GetActorRotation().Yaw - BodyYaw);
		float LagYaw = FMath::Clamp(RawLagYaw, -MaxTurnTwist, MaxTurnTwist);

		// MaxTurnTwist를 넘는 각도는 ±180°에 가까울수록 비틀기를 0으로 줄임
		// → ±180°에서 부호가 바뀌어도 허리가 반대로 튀지 않음 (항상 현재 각도만 사용, 이전 값 누적 없음)
		const float AbsoluteLagYaw = FMath::Abs(RawLagYaw);
		if (AbsoluteLagYaw > MaxTurnTwist && MaxTurnTwist < 180.0f)
		{
			LagYaw = LagYaw * (180.0f - AbsoluteLagYaw) / (180.0f - MaxTurnTwist);
		}

		TurnTwistYawDegrees = LagYaw * AimAlpha;
		AimResidualYawDegrees = FRotator::NormalizeAxis(Character->GetBaseAimRotation().Yaw - (BodyYaw + TurnTwistYawDegrees));
	}

	TurnTwistRotation = FRotator(0.0f, TurnTwistYawDegrees / UpperBodyLayerTurnTwist::BoneCount, 0.0f);
}


// 무기 데이터 에셋에 Aim Offset이 있을 때만 조준 각도만큼 상체를 돌림
// 각도는 AimInterpSpeed로 부드럽게 따라가고, Aim Offset 축 단위(각도 또는 -1~1)로 바꿔서 넘김
void UUpperBodyLayerAnimInstance::UpdateAimOffset(const ACharacter* Character, const UUpperBodyLayer* DataAsset, float DeltaSeconds)
{
	AimOffsetAlpha = 0.0f;
	if (IsValid(DataAsset) == false) return;

	// 좌우는 실제 몸(루트 본) 방향 기준. 캡슐이 먼저 돌고 몸이 제자리 회전으로 따라가는 동안에도 상체가 조준 방향을 봄
	FRotator AimDelta = (Character->GetBaseAimRotation() - Character->GetActorRotation()).GetNormalized();
	const USkeletalMeshComponent* MeshComponent = GetSkelMeshComponent();
	if (IsValid(MeshComponent) == true && MeshComponent->GetNumBones() > 0)
	{
		const float BodyYaw = MeshComponent->GetSocketRotation(MeshComponent->GetBoneName(0)).Yaw - MeshComponent->GetRelativeRotation().Yaw;
		AimDelta.Yaw = FRotator::NormalizeAxis(Character->GetBaseAimRotation().Yaw - BodyYaw);
	}

	const float TargetYaw = FMath::Clamp(AimDelta.Yaw, -MaxAimYaw, MaxAimYaw);
	const float TargetPitch = FMath::Clamp(AimDelta.Pitch, -90.0f, 90.0f);

	if (DataAsset->AimInterpSpeed > KINDA_SMALL_NUMBER)
	{
		AimYawDegrees = FMath::FInterpTo(AimYawDegrees, TargetYaw, DeltaSeconds, DataAsset->AimInterpSpeed);
		AimPitchDegrees = FMath::FInterpTo(AimPitchDegrees, TargetPitch, DeltaSeconds, DataAsset->AimInterpSpeed);
	}
	else
	{
		AimYawDegrees = TargetYaw;
		AimPitchDegrees = TargetPitch;
	}

	// 무기 Aim Offset이 없으면 각도만 계산 (평상시 고개 돌리기에서 사용)
	if (IsValid(DataAsset->AimOffset) == false) return;

	// 몸 → 캡슐 각도는 허리 비틀기가 맡으므로 Aim Offset 좌우는 캡슐 기준 남은 각도만
	CurrentAimOffset = DataAsset->AimOffset;
	const float ActorRelativeYaw = FMath::Clamp(FRotator::NormalizeAxis(Character->GetBaseAimRotation().Yaw - Character->GetActorRotation().Yaw), -MaxAimYaw, MaxAimYaw);
	AimYaw = ConvertToAimOffsetAxis(DataAsset->AimOffset, 0, FMath::Lerp(AimYawDegrees, ActorRelativeYaw, AimAlpha));
	AimPitch = ConvertToAimOffsetAxis(DataAsset->AimOffset, 1, AimPitchDegrees);

	// 조준 자세가 있는 무기는 조준 중에만, 없는 무기는 상체 애니가 섞인 만큼
	AimOffsetAlpha = UpperBodyWeight;
	if (IsValid(DataAsset->AimIdle) == true)
	{
		AimOffsetAlpha = FMath::Min(AimAlpha, UpperBodyWeight);
	}
}


// 조준 중 + 걷기·뛰기 + 조준 이동 데이터베이스 있음일 때만 다리까지 조준 이동 모션매칭으로 섞음
// 서 있을 때는 원래 모션매칭 다리 (제자리 회전 애니 사용)
void UUpperBodyLayerAnimInstance::UpdateAimLocomotion(const UUpperBodyLayer* DataAsset, int32 StateIndex, float DeltaSeconds, float AimBlendSpeed)
{
	UPoseSearchDatabase* TargetDatabase = nullptr;
	if (IsValid(DataAsset) == true)
	{
		TargetDatabase = DataAsset->AimLocomotionDatabase;
	}

	const bool bIsMoving = StateIndex == UpperBodyLayerState::Walk || StateIndex == UpperBodyLayerState::Run;
	float TargetAlpha = 0.0f;
	if (bIsAiming == true && bIsMoving == true && IsValid(TargetDatabase) == true)
	{
		CurrentAimDatabase = TargetDatabase;
		TargetAlpha = 1.0f;
	}

	AimLocomotionAlpha = FMath::FInterpConstantTo(AimLocomotionAlpha, TargetAlpha, DeltaSeconds, AimBlendSpeed);
}


// 평상시 태세(조준 자세가 풀린 만큼)에 고개를 카메라 쪽으로 (모션매칭 Strafe 때 쓰는 고개 Aim Offset, 좌우 ±MaxAimYaw). 공중에서는 끔
// 몸이 이동 방향을 볼 때만 사용. 몸이 카메라를 따라 도는 동안(항상 Strafe 프로젝트 등)은 구동용 ABP가 같은 고개 Aim Offset을 이미 적용하므로 끔 (두 번 돌지 않게)
void UUpperBodyLayerAnimInstance::UpdateRelaxedAimOffset(const UUpperBodyLayer* DataAsset, int32 StateIndex, float DeltaSeconds)
{
	bool bIsBodyFollowingMovement = false;
	const ACharacter* Character = Cast<ACharacter>(TryGetPawnOwner());
	if (IsValid(Character) == true && IsValid(Character->GetCharacterMovement()) == true)
	{
		bIsBodyFollowingMovement = Character->GetCharacterMovement()->bOrientRotationToMovement;
	}

	float TargetAlpha = 0.0f;
	if (IsValid(DataAsset) == true && IsValid(RelaxedAimOffset) == true && StateIndex != UpperBodyLayerState::Air && bIsBodyFollowingMovement == true)
	{
		TargetAlpha = 1.0f - AimAlpha;
		RelaxedAimYaw = ConvertToAimOffsetAxis(RelaxedAimOffset, 0, AimYawDegrees);
		RelaxedAimPitch = ConvertToAimOffsetAxis(RelaxedAimOffset, 1, AimPitchDegrees);
	}

	RelaxedAimAlpha = FMath::FInterpTo(RelaxedAimAlpha, TargetAlpha, DeltaSeconds, 8.0f);
}


// 축 범위가 -1~1 이내면 비율형(각도 / 90), 아니면 각도형으로 보고 축 범위 안으로 자름
float UUpperBodyLayerAnimInstance::ConvertToAimOffsetAxis(const UBlendSpace* AimOffsetAsset, int32 AxisIndex, float Degrees)
{
	const FBlendParameter& Axis = AimOffsetAsset->GetBlendParameter(AxisIndex);

	float AxisValue = Degrees;
	if (FMath::Abs(Axis.Min) <= 1.001f && FMath::Abs(Axis.Max) <= 1.001f)
	{
		AxisValue = Degrees / 90.0f;
	}

	return FMath::Clamp(AxisValue, Axis.Min, Axis.Max);
}


// "구조체변수.bool변수" 또는 "bool변수" 경로를 따라가 값을 읽음 (못 찾으면 false)
bool UUpperBodyLayerAnimInstance::ReadBoolByPath(const UObject* Owner, const FString& PropertyPath)
{
	if (IsValid(Owner) == false) return false;

	TArray<FString> Segments;
	PropertyPath.ParseIntoArray(Segments, TEXT("."), true);
	if (Segments.Num() == 0) return false;

	const UStruct* CurrentStruct = Owner->GetClass();
	const void* CurrentContainer = Owner;
	for (int32 SegmentIndex = 0; SegmentIndex < Segments.Num(); ++SegmentIndex)
	{
		const FProperty* Property = CurrentStruct->FindPropertyByName(FName(*Segments[SegmentIndex]));
		if (Property == nullptr) return false;

		const bool bIsLastSegment = SegmentIndex == Segments.Num() - 1;
		if (bIsLastSegment == true)
		{
			const FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property);
			if (BoolProperty == nullptr) return false;

			return BoolProperty->GetPropertyValue_InContainer(CurrentContainer);
		}

		const FStructProperty* StructProperty = CastField<FStructProperty>(Property);
		if (StructProperty == nullptr) return false;

		CurrentContainer = StructProperty->ContainerPtrToValuePtr<void>(CurrentContainer);
		CurrentStruct = StructProperty->Struct;
	}

	return false;
}


// Enum 변수의 현재 항목 이름 (BP Enum이면 에디터에 보이는 이름)
FName UUpperBodyLayerAnimInstance::ReadEnumEntryName(const UObject* Owner, FName PropertyName)
{
	if (IsValid(Owner) == false) return NAME_None;
	if (PropertyName.IsNone() == true) return NAME_None;

	const FProperty* Property = Owner->GetClass()->FindPropertyByName(PropertyName);
	if (Property == nullptr) return NAME_None;

	const void* ValuePointer = Property->ContainerPtrToValuePtr<void>(Owner);
	const UEnum* Enum = nullptr;
	int64 Value = 0;
	if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
	{
		Enum = EnumProperty->GetEnum();
		Value = EnumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue(ValuePointer);
	}
	else if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
	{
		Enum = ByteProperty->Enum;
		Value = ByteProperty->GetPropertyValue(ValuePointer);
	}

	if (Enum == nullptr) return NAME_None;

	const int32 EntryIndex = Enum->GetIndexByValue(Value);
	if (EntryIndex == INDEX_NONE) return NAME_None;

	return FName(*Enum->GetAuthoredNameStringByIndex(EntryIndex));
}


void UUpperBodyLayerLibrary::SetUpperBodyWeaponMesh(ACharacter* Character, UMeshComponent* WeaponMesh)
{
	if (IsValid(Character) == false) return;

	USkeletalMeshComponent* MeshComponent = Character->GetMesh();
	if (IsValid(MeshComponent) == false) return;

	MeshComponent->ForEachAnimInstance([WeaponMesh](UAnimInstance* AnimInstance)
	{
		auto* UpperBodyInstance = Cast<UUpperBodyLayerAnimInstance>(AnimInstance);
		if (IsValid(UpperBodyInstance) == false) return;

		UpperBodyInstance->SetWeaponMesh(WeaponMesh);
	});
}


bool UUpperBodyLayerLibrary::NotifyUpperBodyFire(ACharacter* Character)
{
	if (IsValid(Character) == false) return false;

	USkeletalMeshComponent* MeshComponent = Character->GetMesh();
	if (IsValid(MeshComponent) == false) return false;

	bool bIsReady = false;
	MeshComponent->ForEachAnimInstance([&bIsReady](UAnimInstance* AnimInstance)
	{
		auto* UpperBodyInstance = Cast<UUpperBodyLayerAnimInstance>(AnimInstance);
		if (IsValid(UpperBodyInstance) == false) return;

		bIsReady = UpperBodyInstance->NotifyFire();
	});

	return bIsReady;
}


void UUpperBodyLayerLibrary::SetUpperBodyZoom(ACharacter* Character, bool bIsZoomHeld)
{
	if (IsValid(Character) == false) return;

	USkeletalMeshComponent* MeshComponent = Character->GetMesh();
	if (IsValid(MeshComponent) == false) return;

	MeshComponent->ForEachAnimInstance([bIsZoomHeld](UAnimInstance* AnimInstance)
	{
		auto* UpperBodyInstance = Cast<UUpperBodyLayerAnimInstance>(AnimInstance);
		if (IsValid(UpperBodyInstance) == false) return;

		UpperBodyInstance->SetZoomHeld(bIsZoomHeld);
	});
}


void UUpperBodyLayerLibrary::SetUpperBodyFireHeld(ACharacter* Character, bool bIsFireHeld)
{
	if (IsValid(Character) == false) return;

	USkeletalMeshComponent* MeshComponent = Character->GetMesh();
	if (IsValid(MeshComponent) == false) return;

	MeshComponent->ForEachAnimInstance([bIsFireHeld](UAnimInstance* AnimInstance)
	{
		auto* UpperBodyInstance = Cast<UUpperBodyLayerAnimInstance>(AnimInstance);
		if (IsValid(UpperBodyInstance) == false) return;

		UpperBodyInstance->SetFireHeld(bIsFireHeld);
	});
}
