#include "GamePlay/Combat/Skill/NFGA_SkillBase.h"

#include "Engine/World.h"
#include "AbilitySystemComponent.h"
#include "TimerManager.h"
#include "Project_NF.h"
#include "GameFramework/Pawn.h"
#include "GamePlay/Combat/Skill/Preview/NF_SkillPreviewActor.h"
#include "GamePlay/Combat/Skill/Execution/NF_SkillExecutionActor.h"
#include "GamePlay/Combat/Components/NF_GroundTargetingComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GamePlay/Combat/Skill/Execution/NF_SkillCastVisual.h"

// 호환 구현: 기존 함수/속성 소유 클래스는 유지하고 구현만 역할별로 분리한다.
bool UNFGA_SkillBase::BeginLocalPreview()
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (!ActorInfo || !ActorInfo->IsLocallyControlled() || !PreviewActorClass
		|| IsActive() || !CanActivateAbility(CurrentSpecHandle, ActorInfo))
	{
		return false;
	}
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!IsValid(Avatar) || !Avatar->GetWorld())
	{
		return false;
	}
	EndLocalPreview();
	FActorSpawnParameters Params;
	Params.Owner = Avatar;
	Params.Instigator = Cast<APawn>(Avatar);
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	LocalPreviewActor = Avatar->GetWorld()->SpawnActor<ANF_SkillPreviewActor>(PreviewActorClass, FTransform::Identity, Params);
	if (!IsValid(LocalPreviewActor) || !LocalPreviewActor->InitializePreview(Avatar, this))
	{
		EndLocalPreview();
		return false;
	}
	return true;
}

void UNFGA_SkillBase::CommitExecute(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::CommitExecute(Handle, ActorInfo, ActivationInfo);
	// All native active skills commit at effect execution, after targeting/cast delay checks.
	// Do not wait for the animation or bind animation completion to EndAbility.
	if (IsActive() && SkillType == ENF_SkillType::Active) { PlayCastMontage(ActorInfo); }
}

void UNFGA_SkillBase::PlayCastMontage(const FGameplayAbilityActorInfo* ActorInfo)
{
	if (!CastMontage || bCastMontageRequested || !ActorInfo || !ActorInfo->IsNetAuthority()) { return; }
	bCastMontageRequested = true;
	AActor* Avatar = ActorInfo->AvatarActor.Get();
	// Dedicated servers need not initialize an AnimInstance to relay a cosmetic to clients.
	USkeletalMeshComponent* Mesh = ActorInfo->SkeletalMeshComponent.Get();
	if (!IsValid(Avatar) || !Avatar->GetWorld() || !IsValid(Mesh))
	{
		UE_LOG(LogProject_NF, Warning, TEXT("[SkillAnimation] 시전 모션 생략: ASC의 애니메이션 메시가 없습니다. GA=%s"), *GetName());
		return;
	}
	ANF_SkillCastVisual::SpawnForCast(Avatar, Mesh->GetFName(), CastMontage, CastMontagePlayRate,
		CastMontageStartSection, ActorInfo->AffectedAnimInstanceTag);
}

void UNFGA_SkillBase::EndLocalPreview()
{
	if (IsValid(LocalPreviewActor))
	{
		LocalPreviewActor->Destroy();
	}
	LocalPreviewActor = nullptr;
}

bool UNFGA_SkillBase::GetSkillPreviewData_Implementation(AActor* SourceAvatar, FNF_SkillPreviewData& OutData) const
{
	if (UsesProjectileExecution()) { return ExecutionActorClass->GetDefaultObject<ANF_SkillExecutionActor>()->GetProjectilePreviewData(SourceAvatar, OutData); }
	OutData = DefaultPreviewData;
	return IsValid(SourceAvatar);
}

void UNFGA_SkillBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CanContinueSkillExecution() || ((bRequiresConfirmedTargetData || UsesProjectileExecution())
		&& (!ActorInfo || !ActorInfo->IsNetAuthority()
			|| !UNFGA_SkillBase::ValidateConfirmedTargetData_Implementation(ConfirmedTargetData)
			|| !ValidateConfirmedTargetData(ConfirmedTargetData))))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (ExecutionActorClass)
	{
		if (!ActorInfo || !ActorInfo->IsNetAuthority()) { EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }
		if (CastTime > 0.0f) { GetWorld()->GetTimerManager().SetTimer(AutomaticCastTimer, this, &UNFGA_SkillBase::StartAutomaticExecution, CastTime, false); }
		else { StartAutomaticExecution(); }
		return;
	}
	// 실행 클래스 미지정 GA의 기존 BP 실행은 유지한다.
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UNFGA_SkillBase::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (!IsEndAbilityValid(Handle, ActorInfo)) { return; }
	if (ScopeLockCount > 0)
	{
		Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
		return;
	}
	bCastMontageRequested = false;
	CancelLocalInput();
	ClearConfirmedTargetData();
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(AutomaticCastTimer);
		GetWorld()->GetTimerManager().ClearTimer(AutomaticEndTimer);
	}
	ANF_SkillExecutionActor* Actor = ActiveExecutionActor;
	ActiveExecutionActor = nullptr;
	if (IsValid(Actor))
	{
		Actor->OnExecutionEnded.RemoveAll(this);
		// Also protect launched/deployed results from EMP when their GA was configured as Timed.
		if (!(bWasCancelled && IsBlockedByEMP() && Actor->SurvivesCasterEMP())) { Actor->StopExecution(); }
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UNFGA_SkillBase::BeginLocalInput()
{
	const FGameplayAbilityActorInfo* Info = GetCurrentActorInfo();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Info || !Info->IsLocallyControlled() || !ASC || !IsValid(Avatar) || !Avatar->GetWorld()
		|| SkillType != ENF_SkillType::Active || IsActive() || !CanContinueSkillExecution()
		|| !CanActivateAbility(CurrentSpecHandle, Info))
	{
		return false;
	}
	TArray<TWeakObjectPtr<UNFGA_SkillBase>> HoldingSkills;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (UNFGA_SkillBase* Other = Cast<UNFGA_SkillBase>(Spec.GetPrimaryInstance()))
		{
			HoldingSkills.Add(Other);
		}
	}
	// BP 취소 이벤트가 GA 목록을 수정할 수 있으므로 목록 순회가 끝난 뒤 호출한다.
	for (const TWeakObjectPtr<UNFGA_SkillBase>& Other : HoldingSkills)
	{
		if (Other.IsValid()) { Other->CancelLocalInput(); }
	}
	const FGameplayAbilitySpec* CurrentSpec = ASC->FindAbilitySpecFromHandle(CurrentSpecHandle);
	if (!CurrentSpec || CurrentSpec->GetPrimaryInstance() != this || !Info->IsLocallyControlled()
		|| GetAvatarActorFromActorInfo() != Avatar || IsActive() || !CanContinueSkillExecution()
		|| !CanActivateAbility(CurrentSpecHandle, Info)) { return false; }
	bLocalInputHolding = true;
	Avatar->GetWorld()->GetTimerManager().SetTimer(LocalInputTimer, this,
		&UNFGA_SkillBase::CheckLocalInputValidity, 0.05f, true);
	if (PreviewActorClass)
	{
		BeginLocalPreview(); // 표시 실패는 입력 자체를 막지 않는다.
	}
	return bLocalInputHolding;
}

bool UNFGA_SkillBase::ConfirmLocalInput(FGameplayAbilityTargetDataHandle& OutTargetData)
{
	OutTargetData.Clear();
	CheckLocalInputValidity();
	if (!bLocalInputHolding)
	{
		return false;
	}
	const bool bValid = BuildConfirmedTargetData(OutTargetData);
	// 정상 확정은 취소 이벤트를 발생시키지 않는다.
	bLocalInputHolding = false;
	CancelLocalInput();
	return bValid;
}

void UNFGA_SkillBase::CancelLocalInput()
{
	const bool bWasHolding = bLocalInputHolding;
	bLocalInputHolding = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LocalInputTimer);
	}
	EndLocalPreview();
	if (bWasHolding)
	{
		LocalInputCanceled();
	}
}

void UNFGA_SkillBase::CheckLocalInputValidity()
{
	const FGameplayAbilityActorInfo* Info = GetCurrentActorInfo();
	if (bLocalInputHolding && (!Info || !Info->IsLocallyControlled()
		|| !IsValid(GetAvatarActorFromActorInfo()) || IsActive() || !CanContinueSkillExecution()
		|| !CanActivateAbility(CurrentSpecHandle, Info)))
	{
		CancelLocalInput();
	}
}

FGameplayAbilityTargetDataHandle UNFGA_SkillBase::MakeSkillTargetData(ENF_SkillTargetKind Kind,
	FVector Location, FVector AimDirection)
{
	FGameplayAbilityTargetDataHandle Result;
	FNF_SkillTargetData* Data = new FNF_SkillTargetData();
	Data->Kind = Kind;
	Data->Location = Location;
	Data->AimDirection = AimDirection.GetSafeNormal();
	Result.Add(Data);
	return Result;
}

bool UNFGA_SkillBase::ReadSkillTargetData(const FGameplayAbilityTargetDataHandle& TargetData,
	ENF_SkillTargetKind& Kind, FVector& Location, FVector& AimDirection)
{
	Kind = ENF_SkillTargetKind::None;
	Location = FVector::ZeroVector;
	AimDirection = FVector::ZeroVector;
	const FNF_SkillTargetData* Data = FNF_SkillTargetData::Get(TargetData);
	if (!Data) { return false; }
	Kind = Data->Kind;
	Location = Data->Location;
	AimDirection = Data->AimDirection;
	return true;
}

bool UNFGA_SkillBase::BuildConfirmedTargetData_Implementation(FGameplayAbilityTargetDataHandle& OutTargetData) const
{
	OutTargetData.Clear();
	if (UsesProjectileExecution())
	{
		AActor* Avatar = GetAvatarActorFromActorInfo();
		if (!IsValid(Avatar)) { return false; }
		FVector Eyes;
		FRotator Aim;
		Avatar->GetActorEyesViewPoint(Eyes, Aim);
		OutTargetData = MakeSkillTargetData(ENF_SkillTargetKind::Direction, FVector::ZeroVector, Aim.Vector());
		return true;
	}
	if (bRequiresConfirmedTargetData && ExecutionActorClass)
	{
		FNF_SkillPreviewData Data;
		FHitResult Hit;
		FVector Direction;
		if (!GetSkillPreviewData(GetAvatarActorFromActorInfo(), Data)
			|| !UNF_GroundTargetingComponent::CalculateGroundTarget(GetAvatarActorFromActorInfo(), Data.TargetRange, Data.GroundSearchDepth, Hit, Direction)) { return false; }
		OutTargetData = MakeSkillTargetData(ENF_SkillTargetKind::GroundPoint, Hit.ImpactPoint, Direction);
		return true;
	}
	return !bRequiresConfirmedTargetData;
}

bool UNFGA_SkillBase::ValidateConfirmedTargetData_Implementation(const FGameplayAbilityTargetDataHandle& TargetData) const
{
	if (TargetData.Num() == 0) { return !bRequiresConfirmedTargetData && !UsesProjectileExecution(); }
	const FNF_SkillTargetData* Data = FNF_SkillTargetData::Get(TargetData);
	if (!Data || Data->Location.ContainsNaN() || Data->AimDirection.ContainsNaN()
		|| !FMath::IsNearlyEqual(Data->AimDirection.SizeSquared(), 1.0, 0.02)) { return false; }
	if (UsesProjectileExecution())
	{
		return Data->Kind == ENF_SkillTargetKind::Direction
			&& ExecutionActorClass->GetDefaultObject<ANF_SkillExecutionActor>()->ValidateProjectileDirection(GetAvatarActorFromActorInfo(), Data->AimDirection);
	}
	if (ExecutionActorClass && bRequiresConfirmedTargetData)
	{
		FNF_SkillPreviewData Preview;
		if (!Data || Data->Kind != ENF_SkillTargetKind::GroundPoint || !GetSkillPreviewData(GetAvatarActorFromActorInfo(), Preview)
			|| !UNF_GroundTargetingComponent::ValidateGroundTarget(GetAvatarActorFromActorInfo(), Data->Location, Preview.TargetRange, Preview.GroundSearchDepth)) { return false; }
	}
	return Data && (Data->Kind == ENF_SkillTargetKind::Direction || Data->Kind == ENF_SkillTargetKind::GroundPoint)
		&& !Data->Location.ContainsNaN() && !Data->AimDirection.ContainsNaN()
		&& FMath::IsNearlyEqual(Data->AimDirection.SizeSquared(), 1.0, 0.02);
}

bool UNFGA_SkillBase::ShouldEndOnInputRelease() const
{
	return ExecutionActorClass && SkillExecutionMode == ENF_SkillExecutionMode::Continuous && !bActivateOnInputRelease;
}

void UNFGA_SkillBase::ReleaseSkillInput()
{
	const FGameplayAbilityActorInfo* Info = GetCurrentActorInfo();
	if (Info && Info->IsNetAuthority() && ShouldEndOnInputRelease() && IsActive())
	{
		EndAbility(CurrentSpecHandle, Info, CurrentActivationInfo, true, false);
	}
}

void UNFGA_SkillBase::StartAutomaticExecution()
{
	if (!IsActive()) { return; }
	const FGameplayAbilityActorInfo* Info = GetCurrentActorInfo();
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Info || !Info->IsNetAuthority() || !IsValid(Avatar) || !ExecutionActorClass || !CanContinueSkillExecution()
		|| !FMath::IsFinite(ExecutionDuration) || (SkillExecutionMode == ENF_SkillExecutionMode::Timed && ExecutionDuration <= 0.0f)
		|| ((bRequiresConfirmedTargetData || UsesProjectileExecution()) && !ValidateConfirmedTargetData(ConfirmedTargetData)))
	{
		EndAbility(CurrentSpecHandle, Info, CurrentActivationInfo, true, true); return;
	}
	FTransform Transform = Avatar->GetActorTransform();
	if (const FNF_SkillTargetData* Target = FNF_SkillTargetData::Get(ConfirmedTargetData))
	{
		if (Target->Kind == ENF_SkillTargetKind::GroundPoint) { Transform = FTransform(FVector(Target->AimDirection).Rotation(), Target->Location); }
	}
	ANF_SkillExecutionActor* Actor = GetWorld()->SpawnActorDeferred<ANF_SkillExecutionActor>(ExecutionActorClass,
		Transform, Avatar, Cast<APawn>(Avatar), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Actor) { EndAbility(CurrentSpecHandle, Info, CurrentActivationInfo, true, true); return; }
	Actor->InitializeExecution(Avatar, this);
	Actor->FinishSpawning(Transform);
	// 조합 검증까지 마친 뒤 Commit한다. 설치/공격 시작은 그 이후다.
	if (!IsValid(Actor) || !Actor->PrepareExecution()
		|| (SkillExecutionMode == ENF_SkillExecutionMode::Instant && !Actor->CanRunIndependently())
		|| !CommitAbility(CurrentSpecHandle, Info, CurrentActivationInfo))
	{
		if (IsValid(Actor)) { Actor->Destroy(); }
		EndAbility(CurrentSpecHandle, Info, CurrentActivationInfo, true, true); return;
	}
	if (!IsActive() || GetAvatarActorFromActorInfo() != Avatar || !CanContinueSkillExecution())
	{
		Actor->Destroy();
		if (IsActive()) { EndAbility(CurrentSpecHandle, Info, CurrentActivationInfo, true, true); }
		return;
	}
	ActiveExecutionActor = Actor;
	Actor->OnExecutionEnded.AddUObject(this, &UNFGA_SkillBase::FinishAutomaticExecution);
	if (!Actor->StartExecution())
	{
		if (IsActive()) { EndAbility(CurrentSpecHandle, Info, CurrentActivationInfo, true, true); }
		return;
	}
	if (!IsActive()) { return; }
	if (SkillExecutionMode == ENF_SkillExecutionMode::Instant)
	{
		// 설치 액터는 생성 후 자신의 수명으로 유지된다.
		Actor->OnExecutionEnded.RemoveAll(this);
		ActiveExecutionActor = nullptr;
		EndAbility(CurrentSpecHandle, Info, CurrentActivationInfo, true, false);
	}
	else if (SkillExecutionMode == ENF_SkillExecutionMode::Timed)
	{
		GetWorld()->GetTimerManager().SetTimer(AutomaticEndTimer, this, &UNFGA_SkillBase::FinishAutomaticExecution, ExecutionDuration, false);
	}
}

void UNFGA_SkillBase::FinishAutomaticExecution()
{
	if (IsActive()) { EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true, false); }
}

bool UNFGA_SkillBase::UsesProjectileExecution() const
{
	return ExecutionActorClass && (ExecutionActorClass->GetDefaultObject<ANF_SkillExecutionActor>()->HasProjectileLaunch()
		|| ExecutionActorClass->GetDefaultObject<ANF_SkillExecutionActor>()->HasGroundTravel());
}

bool UNFGA_SkillBase::PrepareConfirmedTargetData(const FGameplayAbilityTargetDataHandle& TargetData)
{
	const FGameplayAbilityActorInfo* Info = GetCurrentActorInfo();
	ClearConfirmedTargetData();
	if (!Info || !Info->IsNetAuthority() || !CanContinueSkillExecution()
		|| !UNFGA_SkillBase::ValidateConfirmedTargetData_Implementation(TargetData)
		|| !ValidateConfirmedTargetData(TargetData))
	{
		return false;
	}
	ConfirmedTargetData = TargetData;
	return true;
}

void UNFGA_SkillBase::ClearConfirmedTargetData()
{
	ConfirmedTargetData.Clear();
}
